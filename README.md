Create 3D volume from series of scan images.

To create 3D volume you should specify:

1. Path to DICOMDIR file in your DICOM folder(usually something like SOME_SCAN/DICOMDIR). Or just path to the top level folder containing .dcm files, where each dcm contatins one slice.

2. Output folder

3. ISO level for volume creation(-il <value>) - value measured in pixels number. This parameter should be estimated manually, depending on your requireements.

Optional paremeters:

-sbin - binary stl output

-v    - verbose console output

Example command:

dicomtostl.exe -sbin -v -il 100 D:\dicom\BRAIN\DICOMDIR D:\dicom\BRAIN_stl

**Only Windows platform is supported**
