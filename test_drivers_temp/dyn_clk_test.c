#include <linux/module.h>
#include <linux/platform_device.h>
#include <linux/of.h>
#include <linux/clk.h>

static struct clk *dynclk;

static int dynclk_test_probe(struct platform_device *pdev)
{
    int ret;
    unsigned long rate;

    dynclk = devm_clk_get(&pdev->dev, "axi_dynclk");
    if (IS_ERR(dynclk)) {
        dev_err(&pdev->dev, "Failed to get dynclk\n");
        return PTR_ERR(dynclk);
    }

    ret = clk_prepare_enable(dynclk);
    if (ret) {
        dev_err(&pdev->dev, "Failed to enable dynclk\n");
        return ret;
    }

    rate = 74250000; // Set clock to 74.25 MHz
    ret = clk_set_rate(dynclk, rate);
    if (ret) {
        dev_err(&pdev->dev, "Failed to set dynclk rate\n");
        clk_disable_unprepare(dynclk);
        return ret;
    }

    dev_info(&pdev->dev, "dynclk set to %lu Hz\n", clk_get_rate(dynclk));
    return 0;
}

static int dynclk_test_remove(struct platform_device *pdev)
{
    clk_disable_unprepare(dynclk);
    return 0;
}

static const struct of_device_id dynclk_test_of_match[] = {
    { .compatible = "dyn,dynclk-test" },
    { }
};
MODULE_DEVICE_TABLE(of, dynclk_test_of_match);

static struct platform_driver dynclk_test_driver = {
    .driver = {
        .name = "dynclk-test",
        .of_match_table = dynclk_test_of_match,
    },
    .probe = dynclk_test_probe,
    .remove = dynclk_test_remove,
};

module_platform_driver(dynclk_test_driver);

MODULE_LICENSE("GPL");
MODULE_AUTHOR("Selaka");
MODULE_DESCRIPTION("Dummy driver to control Digilent AXI dynclk");
