#include <linux/module.h>
#include <linux/platform_device.h>
#include <linux/of_device.h>
#include <linux/dmaengine.h>
#include <linux/dma-mapping.h>
#include <linux/slab.h>
#include <linux/fs.h>
#include <linux/uaccess.h>
#include <linux/cdev.h>
#include <linux/completion.h>
#include <linux/kernel.h>
#include <linux/timekeeping.h>

#define DEVICE_NAME "fft_dma"
#define DMA_BUF_WORDS 1024        // Number of 64-bit samples
#define DMA_BUF_SIZE (DMA_BUF_WORDS * sizeof(u64))

struct fft_dma_dev {
    struct dma_chan *s2mm_chan;
    dma_addr_t s2mm_dma_handle;
    u64 *s2mm_buf;
    struct device *dev;
    struct completion s2mm_cmp;

    struct cdev cdev;
    dev_t devt;
};

static struct class *fft_dma_class;

static void dma_s2mm_callback(void *data)
{
    struct fft_dma_dev *priv = data;
    complete(&priv->s2mm_cmp);
}

static ssize_t fft_dma_read(struct file *file, char __user *buf, size_t count, loff_t *ppos)
{
    struct fft_dma_dev *priv = file->private_data;
    struct dma_async_tx_descriptor *rx_desc;
    dma_cookie_t cookie;
    size_t words = DMA_BUF_WORDS; // Always transfer 1024 samples
    ktime_t start, end;
    s64 delta_ns;
    u64 delta_us;

    if (count < DMA_BUF_SIZE)
        return -EINVAL; // User buffer too small

    reinit_completion(&priv->s2mm_cmp);

    /* Prepare DMA descriptor */
    rx_desc = dmaengine_prep_slave_single(priv->s2mm_chan,
                                          priv->s2mm_dma_handle,
                                          DMA_BUF_SIZE,
                                          DMA_DEV_TO_MEM,
                                          DMA_PREP_INTERRUPT);
    if (!rx_desc)
        return -EIO;

    rx_desc->callback = dma_s2mm_callback;
    rx_desc->callback_param = priv;

    start = ktime_get();

    cookie = dmaengine_submit(rx_desc);
    dma_async_issue_pending(priv->s2mm_chan);

    if (!wait_for_completion_timeout(&priv->s2mm_cmp, msecs_to_jiffies(10000)))
        return -ETIMEDOUT;

    end = ktime_get();

    delta_ns = ktime_to_ns(ktime_sub(end, start));
    delta_us = delta_ns;
    do_div(delta_us, 1000);

    /* Copy data to user space */
    if (copy_to_user(buf, priv->s2mm_buf, DMA_BUF_SIZE))
        return -EFAULT;

    printk(KERN_INFO "[READ] DMA transfer completed in %llu us\n", delta_us);
    return DMA_BUF_SIZE;
}

static int fft_dma_open(struct inode *inode, struct file *file)
{
    struct fft_dma_dev *priv = container_of(inode->i_cdev, struct fft_dma_dev, cdev);
    file->private_data = priv;
    return 0;
}

static int fft_dma_release(struct inode *inode, struct file *file)
{
    return 0;
}

static const struct file_operations fft_dma_fops = {
    .owner = THIS_MODULE,
    .open = fft_dma_open,
    .release = fft_dma_release,
    .read = fft_dma_read,
};

static int fft_dma_probe(struct platform_device *pdev)
{
    struct fft_dma_dev *priv;
    struct device *dev = &pdev->dev;
    int ret;

    priv = devm_kzalloc(dev, sizeof(*priv), GFP_KERNEL);
    if (!priv)
        return -ENOMEM;

    priv->dev = dev;

    priv->s2mm_buf = dma_alloc_coherent(dev, DMA_BUF_SIZE, &priv->s2mm_dma_handle, GFP_KERNEL);
    if (!priv->s2mm_buf)
        return -ENOMEM;

    priv->s2mm_chan = dma_request_chan(dev, "axidma_s2mm");
    if (IS_ERR(priv->s2mm_chan)) {
        ret = PTR_ERR(priv->s2mm_chan);
        goto err_dma_free;
    }

    init_completion(&priv->s2mm_cmp);
    platform_set_drvdata(pdev, priv);

    ret = alloc_chrdev_region(&priv->devt, 0, 1, DEVICE_NAME);
    if (ret < 0)
        goto err_dma_release;

    cdev_init(&priv->cdev, &fft_dma_fops);
    priv->cdev.owner = THIS_MODULE;

    ret = cdev_add(&priv->cdev, priv->devt, 1);
    if (ret)
        goto err_unregister_chrdev;

    fft_dma_class = class_create(DEVICE_NAME);
    if (IS_ERR(fft_dma_class)) {
        ret = PTR_ERR(fft_dma_class);
        goto err_cdev_del;
    }

    if (!device_create(fft_dma_class, dev, priv->devt, NULL, DEVICE_NAME)) {
        ret = -ENOMEM;
        goto err_class_destroy;
    }

    return 0;

err_class_destroy:
    class_destroy(fft_dma_class);
err_cdev_del:
    cdev_del(&priv->cdev);
err_unregister_chrdev:
    unregister_chrdev_region(priv->devt, 1);
err_dma_release:
    dma_release_channel(priv->s2mm_chan);
err_dma_free:
    dma_free_coherent(dev, DMA_BUF_SIZE, priv->s2mm_buf, priv->s2mm_dma_handle);
    return ret;
}

static int fft_dma_remove(struct platform_device *pdev)
{
    struct fft_dma_dev *priv = platform_get_drvdata(pdev);

    device_destroy(fft_dma_class, priv->devt);
    class_destroy(fft_dma_class);
    cdev_del(&priv->cdev);
    unregister_chrdev_region(priv->devt, 1);

    if (priv->s2mm_chan)
        dma_release_channel(priv->s2mm_chan);
    dma_free_coherent(priv->dev, DMA_BUF_SIZE, priv->s2mm_buf, priv->s2mm_dma_handle);

    return 0;
}

static const struct of_device_id fft_dma_of_match[] = {
    { .compatible = "dma_test,fft-dma" },
    { /* sentinel */ }
};
MODULE_DEVICE_TABLE(of, fft_dma_of_match);

static struct platform_driver fft_dma_driver = {
    .probe = fft_dma_probe,
    .remove = fft_dma_remove,
    .driver = {
        .name = "fft_dma",
        .of_match_table = fft_dma_of_match,
    },
};

static int __init fft_dma_init(void)
{
    return platform_driver_register(&fft_dma_driver);
}

static void __exit fft_dma_exit(void)
{
    platform_driver_unregister(&fft_dma_driver);
}

module_init(fft_dma_init);
module_exit(fft_dma_exit);

MODULE_LICENSE("GPL");
MODULE_AUTHOR("Selaka Deemantha");
MODULE_DESCRIPTION("FFT IP DMA driver for 1024x64-bit samples using S2MM channel");

