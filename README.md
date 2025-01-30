This repository stores the code for the esp32 sink device.

The sink device receives incoming audio from the audio source and passes it onto the bt source device over I2S.

No audio processing is done in the code, only transmission of the audio through the pipeline.

AUDIO SOURCE --> RINGBUFF --> I2S tx