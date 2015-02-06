*****************************
Intrinsic groups
*****************************

======================================
openMVG intrinsic group explaination
======================================

openMVG groups pictures that share common intrinsic parameter to make parameters estimation more stable.
So each camera will have it's own extrinsic parameter group but can share intrinsic parameters with an image collection.

Intrinsic image analysis is made from JPEG EXIF metadata and a database of camera sensor width.
If you have image with no metadata you can specify the known pixel focal length value directly.

Intrinsic analysis and export:
-----------------------------------

The process exports in outputDirectory/**imageParams.json** file the extracted camera intrinsics.

  .. code-block:: c++

    $ cd ./software/SfM/Release/
    $ openMVG_main_CreateList [-i|--imageDirectory] [-d|--sensorWidthDatabase] [-o|--outputDirectory] [-f|--focal]

  - Usage of the automatic chain (with JPEG images)
  
  .. code-block:: c++
  
    $ openMVG_main_CreateList -i Dataset/images/ -o Dataset/matches/ -d ./software/SfM/cameraSensorWidth/cameraGenerated.txt


  The focal in pixel is computed according the following formula:

    .. math::
      
      \text{focal}_{pix} = \frac{max( w_\text{pix}, h_\text{pix} ) * \text{focal}_\text{mm}} {\text{ccdw}_\text{mm}}

    - :math:`\text{focal}_{pix}` the EXIF focal length (pixels),
    - :math:`\text{focal}_{mm}` the EXIF focal length (mm),
    - :math:`w_\text{pix}, h_\text{pix}` the image of width and height (pixels),
    - :math:`\text{ccdw}_\text{mm}` the known sensor width size (mm)

      - used if the EXIF camera model and maker is found in the sensorWidthDatabase database openMVG/src/software/SfM/cameraSensorWidth/cameraGenerated.txt

  - If all the camera have the same focal length or no EXIF info, you can set the pixel focal length directly
  
  .. code-block:: c++
  
    $ openMVG_main_CreateList -f 2750(i.e in pixels) -i Dataset/images/ -o Dataset/matches/

Depending if EXIF data and if the camera model and make is registered in the camera databset, per lines the following information are displayed:

  - no information can be extracted (only the principal point position is exported)
  - the image contain EXIF Jpeg approximated focal length
    
    - if the camera sensor is saved in the openMVG database the focal length and principal point is exported
    - else no focal camera can be computed, only the image size is exported.


Example of a imageParams.json file

  .. code-block:: json

    [
      {
          "filename": "2015-02-05 11.50.17.jpg",
          "width": 2448,
          "height": 3264,
          "focalLength": 2000.0,
          "camera": {
              "name": "Apple",
              "model": "iPhone 5s"
          }
      },
      {
          "filename": "2015-02-05 11.50.21.jpg",
          "width": 2448,
          "height": 3264,
          "focalLength": 2000.0,
          "camera": {
              "name": "Apple",
              "model": "iPhone 5s"
          }
      }
    ]
