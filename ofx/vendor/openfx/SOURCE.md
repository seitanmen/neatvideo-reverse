# OpenFX headers (vendored)

- Source: DaVinci Resolve developer kit, `/Library/Application Support/Blackmagic Design/DaVinci Resolve/Developer/OpenFX/OpenFX-1.4/include/`
  (README.txt of the kit: "Updated as of 12 May 2026"; the headers of the OpenFX project, https://github.com/AcademySoftwareFoundation/openfx).
- Licence: BSD-3-Clause (each file: `Copyright OpenFX and contributors to the OpenFX project.` / `SPDX-License-Identifier: BSD-3-Clause`);
  the licence text is in LICENSE. Only the eight headers the plug-in and the test host include were copied, unmodified.
  Not copied: the Resolve extension headers without a licence notice (ofxImageEffectExt.h, ofxParamExt.h) and the unused ones.
- Copied 2026-10-31 (N4a). SHA-256:

```
d5bb636036e40e718c80e2451085771b405db242b93584552877e2f9dc2f5ecf  ofxCore.h
b02871a8bbe5e48a15de88a77e555a769ea97b692ab5ca03c5aba9e98ad8553d  ofxImageEffect.h
2db9643bd6b2ce346b395b880202709fcf0f68895bed6fdbc104a3842dc10c97  ofxInteract.h
ec2a83380018a6a981314e624fc573d29a8e4132731676a3cccc1160ad263c6b  ofxMemory.h
63bb85a5c99ad7e19276070ae7d68417ad5a9ab00d3077d16e72c50192e005e1  ofxMessage.h
034bffcef1d4a9c8de4b0c548097396064d56db195b1cc5874c168dc68cc0107  ofxMultiThread.h
8b9e685616ab4abb59e05243f0f9863ed1f5dec2ca270323f25e94deca2dc078  ofxParam.h
82e29cbdd626838cce82930f03a002f528f7deace6ba386429e090bd5a20e57a  ofxProperty.h
```
