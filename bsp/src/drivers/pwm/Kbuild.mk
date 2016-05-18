ifeq ($(CONFIG_INTEL_QRK_PWM),y)
obj-y += intel_qrk_pwm.o
obj-$(CONFIG_TCMD_PWM) += pwm_tcmd.o
endif
