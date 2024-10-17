List of Scanning sonar Commands (Firmware 1.0.4)
================================================

| CommandID | Description | Value | Items | Comment |
|:---|---|:---:|:---:|---|
| IdCentralFrequency | Set transmitting Frequency | 100000~1000000, 0 - Default | Hz | Transmitting frequency |
| IdFrequencyBand | Set Frequency Band | 1~100000, 0 - Default | Hz | Transmitting frequency band |
| IdToneChirp | Tone or Chirp | 0 - tone(CW), 1 - Chirp(FM), 2 - Chirp(AFM) | N/A | Tone or Chirp signal selection |
| IdTxLength | Set transmit pulse length | 10~200 | µseconds | Pulse length in microsecond |
| IdGain | Set receiver's gain | ±60 | dB | Gain |
| IdCommandID | Set command ID | Any | N/A | See Command ID description |
| IdSamples | Set number of samples | 240~13340 | N/A | Number of returning samples (with 100kHz sample rate) |
| IdSamplFreq | Sampling frequency | 100000 | Hz | Now only 100kHz sampling frequency is supported |
| IdTVGTime | TVG's DAC sampling time | 80 | us | Default value is 80us, do not alter |
| IdSectorHeading | Sector's mode heading | 0~28800 | N/A | Heading value for sector mode |
| IdSectorWidth | Sector's mode width | 0~28800 | N/A | Width value for sector mode, 0 - continious mode |
| IdRotationParam | Rotation direction | 0 - CW, 1 - CCW | N/A | Rotation direction |
| IdSteppingMode | Stepping mode | 0 - Stop, 1 - 0.1125°, 2 - 0.225°, 4 - 0.45°, 8 - 0.9°, 16 - 1.8° | N/A | Rotation angle |
| IdSteppingAngle | Stepping angle | 0 | N/A | Must be 0 |
