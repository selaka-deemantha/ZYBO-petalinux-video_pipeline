static int digilent_hdmi_get_modes(struct drm_connector *connector)
static int digilent_hdmi_mode_valid(struct drm_connector *connector, struct drm_display_mode *mode)
static struct drm_encoder *digilent_hdmi_best_encoder(struct drm_connector *connector)

static struct drm_connector_helper_funcs digilent_hdmi_connector_helper_funcs = {
	.get_modes = digilent_hdmi_get_modes,
	.mode_valid = digilent_hdmi_mode_valid,
	.best_encoder = digilent_hdmi_best_encoder,
};

static enum drm_connector_status digilent_hdmi_detect(struct drm_connector *connector, bool force)
static void digilent_hdmi_connector_destroy(struct drm_connector *connector)

static const struct drm_connector_funcs digilent_hdmi_connector_funcs = {
	.detect = digilent_hdmi_detect,
	.fill_modes = drm_helper_probe_single_connector_modes,
	.destroy = digilent_hdmi_connector_destroy,
	.atomic_duplicate_state = drm_atomic_helper_connector_duplicate_state,
	.atomic_destroy_state = drm_atomic_helper_connector_destroy_state,
	.reset = drm_atomic_helper_connector_reset,
};

static int digilent_hdmi_create_connector(struct digilent_hdmi *hdmi)


static void digilent_hdmi_atomic_mode_set(struct drm_encoder *encoder, struct drm_crtc_state *crtc_state, struct drm_connector_state *connector_state)


static void digilent_hdmi_enable(struct drm_encoder *encoder)

static void digilent_hdmi_disable(struct drm_encoder *encoder)

static const struct drm_encoder_helper_funcs digilent_hdmi_encoder_helper_funcs = {
	.atomic_mode_set = digilent_hdmi_atomic_mode_set,
	.enable = digilent_hdmi_enable,
	.disable = digilent_hdmi_disable,
};

static const struct drm_encoder_funcs digilent_hdmi_encoder_funcs = {
	.destroy = drm_encoder_cleanup,
};

static int digilent_hdmi_create_encoder(struct digilent_hdmi *hdmi)

static int digilent_hdmi_parse_dt(struct digilent_hdmi *hdmi)

static int digilent_hdmi_bind(struct device *dev, struct device *master, void *data)


static void digilent_hdmi_unbind(struct device *dev, struct device *master, void *data)

static const struct component_ops digilent_hdmi_component_ops = {
	.bind = digilent_hdmi_bind,
	.unbind = digilent_hdmi_unbind,
};

static int digilent_hdmi_probe(struct platform_device *pdev)

static int digilent_hdmi_remove(struct platform_device *pdev)



static const struct of_device_id digilent_hdmi_of_match[] = {
	{ .compatible = "digilent,hdmi" },
	{}
};

MODULE_DEVICE_TABLE(of, digilent_hdmi_of_match);

static struct platform_driver hdmi_driver = {
	.probe = digilent_hdmi_probe,
	.remove = digilent_hdmi_remove,
	.driver = {
		.name = "digilent-hdmi",
		.of_match_table = digilent_hdmi_of_match,
	},
};






