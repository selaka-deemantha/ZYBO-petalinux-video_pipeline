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

#define DEVICE_NAME "fft_dma"
#define DMA_BUF_WORDS 1024
#define DMA_BUF_SIZE (DMA_BUF_WORDS * sizeof(u32))

struct fft_dma_dev {
    struct dma_chan *s2mm_chan;
    dma_addr_t s2mm_dma_handle;
    u32 *s2mm_buf;
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
    size_t words = min(count / sizeof(u32), (size_t)DMA_BUF_WORDS);
    ktime_t start, end;
    s64 delta_ns;
    u64 delta_us;

    reinit_completion(&priv->s2mm_cmp);
    dev_info(priv->dev, "[READ] Preparing DMA transfer for %zu words\n", words);

    /* Prepare the DMA descriptor */
    rx_desc = dmaengine_prep_slave_single(priv->s2mm_chan,
                                          priv->s2mm_dma_handle,
                                          words * sizeof(u32),
                                          DMA_DEV_TO_MEM,
                                          DMA_PREP_INTERRUPT);
    if (!rx_desc) {
        dev_err(priv->dev, "[READ] Failed to prepare DMA descriptor\n");
        return -EIO;
    }

    rx_desc->callback = dma_s2mm_callback;
    rx_desc->callback_param = priv;

    start = ktime_get();

    cookie = dmaengine_submit(rx_desc);
    dma_async_issue_pending(priv->s2mm_chan);

    dev_info(priv->dev, "[READ] DMA issued, waiting for completion...\n");

    if (!wait_for_completion_timeout(&priv->s2mm_cmp, msecs_to_jiffies(10000))) {
        dev_err(priv->dev, "[READ] DMA timeout!\n");
        return -ETIMEDOUT;
    }

    end = ktime_get();

    delta_ns = ktime_to_ns(ktime_sub(end, start));
    delta_us = delta_ns;
    do_div(delta_us, 1000); /* divide 64-bit value safely in kernel */

    dev_info(priv->dev, "[READ] DMA transfer completed in %lld ns (%llu us)\n",
             delta_ns, delta_us);

    /* Copy data to user space */
    if (copy_to_user(buf, priv->s2mm_buf, words * sizeof(u32))) {
        dev_err(priv->dev, "[READ] Failed to copy data to user space\n");
        return -EFAULT;
    }

    dev_info(priv->dev, "[READ] DMA read complete, %zu bytes transferred\n",
             words * sizeof(u32));
    return words * sizeof(u32);
}


static int fft_dma_open(struct inode *inode, struct file *file)
{
    struct fft_dma_dev *priv = container_of(inode->i_cdev, struct fft_dma_dev, cdev);
    file->private_data = priv;
    dev_info(priv->dev, "[OPEN] Device opened\n");
    return 0;
}

static int fft_dma_release(struct inode *inode, struct file *file)
{
    struct fft_dma_dev *priv = file->private_data;
    dev_info(priv->dev, "[RELEASE] Device closed\n");
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

    dev_info(dev, "[PROBE] FFT DMA driver probing\n");

    priv = devm_kzalloc(dev, sizeof(*priv), GFP_KERNEL);
    if (!priv)
        return -ENOMEM;

    priv->dev = dev;

    priv->s2mm_buf = dma_alloc_coherent(dev, DMA_BUF_SIZE, &priv->s2mm_dma_handle, GFP_KERNEL);
    if (!priv->s2mm_buf) {
        dev_err(dev, "[PROBE] Failed to allocate DMA buffer\n");
        return -ENOMEM;
    }
    dev_info(dev, "[PROBE] DMA buffer allocated at virt=%p phys=%pad\n",
             priv->s2mm_buf, &priv->s2mm_dma_handle);

    priv->s2mm_chan = dma_request_chan(dev, "axidma_s2mm");
    if (IS_ERR(priv->s2mm_chan)) {
        dev_err(dev, "[PROBE] Failed to request DMA channel\n");
        ret = PTR_ERR(priv->s2mm_chan);
        goto err_dma_free;
    }

    init_completion(&priv->s2mm_cmp);
    platform_set_drvdata(pdev, priv);

    ret = alloc_chrdev_region(&priv->devt, 0, 1, DEVICE_NAME);
    if (ret < 0) {
        dev_err(dev, "[PROBE] Failed to allocate char device region\n");
        goto err_dma_release;
    }

    cdev_init(&priv->cdev, &fft_dma_fops);
    priv->cdev.owner = THIS_MODULE;

    ret = cdev_add(&priv->cdev, priv->devt, 1);
    if (ret) {
        dev_err(dev, "[PROBE] Failed to add cdev\n");
        goto err_unregister_chrdev;
    }

    fft_dma_class = class_create(DEVICE_NAME);
    if (IS_ERR(fft_dma_class)) {
        dev_err(dev, "[PROBE] Failed to create device class\n");
        ret = PTR_ERR(fft_dma_class);
        goto err_cdev_del;
    }

    if (!device_create(fft_dma_class, dev, priv->devt, NULL, DEVICE_NAME)) {
        dev_err(dev, "[PROBE] Failed to create device node\n");
        ret = -ENOMEM;
        goto err_class_destroy;
    }

    dev_info(dev, "[PROBE] FFT DMA device created at /dev/%s\n", DEVICE_NAME);
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

    dev_info(priv->dev, "[REMOVE] Removing FFT DMA driver\n");

    device_destroy(fft_dma_class, priv->devt);
    class_destroy(fft_dma_class);
    cdev_del(&priv->cdev);
    unregister_chrdev_region(priv->devt, 1);

    if (priv->s2mm_chan)
        dma_release_channel(priv->s2mm_chan);
    dma_free_coherent(priv->dev, DMA_BUF_SIZE, priv->s2mm_buf, priv->s2mm_dma_handle);

    dev_info(priv->dev, "[REMOVE] FFT DMA driver removed\n");
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
    int ret;

    dev_info(NULL, "[INIT] Registering FFT DMA platform driver\n");

    ret = platform_driver_register(&fft_dma_driver);
    if (ret)
        pr_err("[INIT] Failed to register platform driver\n");

    return ret;
}

static void __exit fft_dma_exit(void)
{
    platform_driver_unregister(&fft_dma_driver);
    pr_info("[EXIT] FFT DMA driver unregistered\n");
}

module_init(fft_dma_init);
module_exit(fft_dma_exit);

MODULE_LICENSE("GPL");
MODULE_AUTHOR("Selaka Deemantha");
MODULE_DESCRIPTION("FFT IP DMA driver using S2MM channel with debug messages");
