Import("env")

# Align PlatformIO artifact names with Keil target output names.
pioenv = env.get("PIOENV", "")

if pioenv == "temperatureSensor-Debug":
	env.Replace(PROGNAME="WaterTempControl_temperatureSensor-Debug")
elif pioenv == "temperatureSensor-Release" or pioenv == "stm32f103rb_hal" or pioenv == "Release":
	env.Replace(PROGNAME="WaterTempControl_temperatureSensor-Release")
elif pioenv == "edgegateway-Debug":
	env.Replace(PROGNAME="WaterTempControl_edgegateway-Debug")
elif pioenv == "edgegateway-Release" or pioenv == "edgegateway_hal":
	env.Replace(PROGNAME="WaterTempControl_edgegateway-Release")
else:
	env.Replace(PROGNAME="WaterTempControl")
