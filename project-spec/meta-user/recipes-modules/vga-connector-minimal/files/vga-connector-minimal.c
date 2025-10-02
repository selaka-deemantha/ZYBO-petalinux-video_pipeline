// SPDX-License-Identifier: GPL-2.0
/*
 * Minimal PL VGA connector/encoder driver
 *
 * Provides a simple fixed-timing DRM connector for a PL video pipeline.
 * Intended for use on Zynq/Zybo/Zedboard type platforms.
 */

#include <linux/module.h>
#include <linux/platform_device.h>
#include <linux/component.h>
#include <linux/of.h>
#include <linux/clk.h>

#include <drm/drm_fourcc.h>
#include <drm/drm_edid.h>
#include <drm/drm_atomic_helper.h>
#include <drm/drm_crtc.h>
#include <drm/drm_modes.h>
#include <drm/drm_encoder.h>
#include <drm/drm_connector.h>
#include <drm/drm_probe_helper.h>
#define DRV_PREFIX "[pl_vga] "

struct pl_vga {
	struct drm_encoder encoder;
	struct drm_connector connector;
	struct drm_device *drm_dev;
	struct device *dev;
	struct clk *clk;
	bool clk_enabled;
	u32 hmax;
	u32 vmax;
};

#define connector_to_vga(c) container_of(c, struct pl_vga, connector)
#define encoder_to_vga(e)   container_of(e, struct pl_vga, encoder)

/* ---------- Connector helpers ---------- */

static int pl_vga_get_modes(struct drm_connector *connector)
{
	struct pl_vga *vga = connector_to_vga(connector);
	int count;

	count = drm_add_modes_noedid(connector, vga->hmax, vga->vmax);
	drm_set_preferred_mode(connector, vga->hmax, vga->vmax);
	return count;
}

static int pl_vga_mode_valid(struct drm_connector *connector,
			     struct drm_display_mode *mode)
{
	struct pl_vga *vga = connector_to_vga(connector);

	if (mode->hdisplay > vga->hmax || mode->vdisplay > vga->vmax)
		return MODE_BAD;

	return MODE_OK;
}

static struct drm_encoder *pl_vga_best_encoder(struct drm_connector *connector)
{
	struct pl_vga *vga = connector_to_vga(connector);
	return &vga->encoder;
}

static const struct drm_connector_helper_funcs pl_vga_connector_helper_funcs = {
	.get_modes   = pl_vga_get_modes,
	.mode_valid  = pl_vga_mode_valid,
	.best_encoder= pl_vga_best_encoder,
};

static enum drm_connector_status
pl_vga_detect(struct drm_connector *connector, bool force)
{
	/* We always report “connected” (no hotplug) */
	return connector_status_connected;
}

static void pl_vga_connector_destroy(struct drm_connector *connector)
{
	drm_connector_unregister(connector);
	drm_connector_cleanup(connector);
}

static const struct drm_connector_funcs pl_vga_connector_funcs = {
	.detect                   = pl_vga_detect,
	.fill_modes               = drm_helper_probe_single_connector_modes,
	.destroy                  = pl_vga_connector_destroy,
	.reset                     = drm_atomic_helper_connector_reset,
	.atomic_duplicate_state    = drm_atomic_helper_connector_duplicate_state,
	.atomic_destroy_state      = drm_atomic_helper_connector_destroy_state,
};

/* ---------- Encoder helpers ---------- */

static void pl_vga_atomic_mode_set(struct drm_encoder *encoder,
				   struct drm_crtc_state *crtc_state,
				   struct drm_connector_state *conn_state)
{
	struct pl_vga *vga = encoder_to_vga(encoder);
	struct drm_display_mode *m = &crtc_state->adjusted_mode;

	if (vga->clk) {
		unsigned long target = (unsigned long)m->clock * 1000UL; /* kHz → Hz */
		clk_set_rate(vga->clk, target);
	}
}

static void pl_vga_enable(struct drm_encoder *encoder)
{
	struct pl_vga *vga = encoder_to_vga(encoder);

	if (vga->clk && !vga->clk_enabled) {
		if (!clk_prepare_enable(vga->clk))
			vga->clk_enabled = true;
	}
}

static void pl_vga_disable(struct drm_encoder *encoder)
{
	struct pl_vga *vga = encoder_to_vga(encoder);

	if (vga->clk && vga->clk_enabled) {
		clk_disable_unprepare(vga->clk);
		vga->clk_enabled = false;
	}
}

static const struct drm_encoder_helper_funcs pl_vga_encoder_helper_funcs = {
	.atomic_mode_set = pl_vga_atomic_mode_set,
	.enable          = pl_vga_enable,
	.disable         = pl_vga_disable,
};

static const struct drm_encoder_funcs pl_vga_encoder_funcs = {
	.destroy = drm_encoder_cleanup,
};

/* ---------- Component bind/unbind ---------- */

static int pl_vga_create_encoder(struct pl_vga *vga)
{
	int ret;

	vga->encoder.possible_crtcs = 1;
	ret = drm_encoder_init(vga->drm_dev, &vga->encoder,
			       &pl_vga_encoder_funcs,
			       DRM_MODE_ENCODER_DAC, NULL);
	if (ret)
		return ret;

	drm_encoder_helper_add(&vga->encoder, &pl_vga_encoder_helper_funcs);
	return 0;
}

static int pl_vga_create_connector(struct pl_vga *vga)
{
	struct drm_connector *connector = &vga->connector;
	int ret;

	connector->polled = DRM_CONNECTOR_POLL_CONNECT | DRM_CONNECTOR_POLL_DISCONNECT;

	ret = drm_connector_init(vga->drm_dev, connector,
				 &pl_vga_connector_funcs,
				 DRM_MODE_CONNECTOR_VGA);
	if (ret)
		return ret;

	drm_connector_helper_add(connector, &pl_vga_connector_helper_funcs);
	drm_connector_register(connector);
	drm_connector_attach_encoder(connector, &vga->encoder);
	return 0;
}

static int pl_vga_bind(struct device *dev, struct device *master, void *data)
{
	struct pl_vga *vga = dev_get_drvdata(dev);
	int ret;

	vga->drm_dev = data;

	ret = pl_vga_create_encoder(vga);
	if (ret)
		return ret;

	ret = pl_vga_create_connector(vga);
	if (ret) {
		drm_encoder_cleanup(&vga->encoder);
		return ret;
	}
	return 0;
}

static void pl_vga_unbind(struct device *dev, struct device *master, void *data)
{
	struct pl_vga *vga = dev_get_drvdata(dev);
	pl_vga_disable(&vga->encoder);
}

/* ---------- Platform driver ---------- */

static const struct component_ops pl_vga_component_ops = {
	.bind   = pl_vga_bind,
	.unbind = pl_vga_unbind,
};

static int pl_vga_probe(struct platform_device *pdev)
{
	struct pl_vga *vga;
	int ret;

	vga = devm_kzalloc(&pdev->dev, sizeof(*vga), GFP_KERNEL);
	if (!vga)
		return -ENOMEM;

	vga->dev = &pdev->dev;
	vga->clk = devm_clk_get(&pdev->dev, "clk");
	if (IS_ERR(vga->clk))
		vga->clk = NULL;

	u32 val;
	if (of_property_read_u32(pdev->dev.of_node, "max-hres", &val))
	    val = 1280;   
	vga->hmax = val;

	if (of_property_read_u32(pdev->dev.of_node, "max-vres", &val))
	    val = 720;
	vga->vmax = val;

	platform_set_drvdata(pdev, vga);

	ret = component_add(&pdev->dev, &pl_vga_component_ops);
	if (ret)
		return ret;

	return 0;
}

static int pl_vga_remove(struct platform_device *pdev)
{
	component_del(&pdev->dev, &pl_vga_component_ops);
	return 0;
}

static const struct of_device_id pl_vga_of_match[] = {
	{ .compatible = "vga-connector-minimal" },
	{ /* sentinel */ }
};
MODULE_DEVICE_TABLE(of, pl_vga_of_match);

static struct platform_driver pl_vga_driver = {
	.probe  = pl_vga_probe,
	.remove = pl_vga_remove,
	.driver = {
		.name           = "pl-vga",
		.of_match_table = pl_vga_of_match,
	},
};
module_platform_driver(pl_vga_driver);

MODULE_LICENSE("GPL v2");
MODULE_AUTHOR("Open-Source / Adapted by ChatGPT");
MODULE_DESCRIPTION("Minimal PL VGA connector driver for DRM/KMS");

