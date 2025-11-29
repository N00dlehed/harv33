"""Central list of MQTT topics for HARV-33."""

class Topics:
    """Defines MQTT topics for brain, bridge, and body modules."""

    SENSORS_ALL = "harv/body/sensors"
    ACTION_IDLE = "harv/body/action/idle"
    ACTION_MOVE = "harv/body/action/move"
    ACTION_LEDS = "harv/body/action/leds"
    BRIDGE_UART_IN = "harv/bridge/uart/in"
    BRIDGE_UART_OUT = "harv/bridge/uart/out"
    CAMERA_STREAM = "harv/camera/stream"
    LOGS = "harv/logs"
