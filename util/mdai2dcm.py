import argparse
import distutils.spawn
import os
import pandas as pd
import numpy as np
import mdai
import logging
import pydicom
import tqdm
import itk
import cv2
import copy
import glob
import subprocess
import glob

# logging setup

logging.basicConfig()

# By default the root logger is set to WARNING and all loggers you define
# inherit that value. Here we set the root logger to NOTSET. This logging
# level is automatically inherited by all existing and new sub-loggers
# that do not set a less verbose level.
logging.root.setLevel(logging.NOTSET)

# The following line sets the root logger level as well.
# It's equivalent to both previous statements combined:
logging.basicConfig(level=logging.NOTSET)

logger = logging.getLogger('dcmqi.mdai2dcm')
logger.setLevel(logging.ERROR)

# Helper functions
dcmqi_template =  {
  "ContentCreatorName": "",
  "ClinicalTrialSeriesID": "Session1",
  "ClinicalTrialTimePointID": "1",
  "SeriesDescription": "Segmentation",
  "SeriesNumber": "300",
  "InstanceNumber": "1",
  "BodyPartExamined": "CHEST",
  "segmentAttributes": [

  ],
  "ContentLabel": "SEGMENTATION",
  "ContentDescription": "Image segmentation",
  "ClinicalTrialCoordinatingCenterName": "dcmqi"
}

def makeHash(text, length=6):
    from base64 import b64encode
    from hashlib import sha1
    return b64encode(sha1(str.encode(text)).digest()).decode('ascii')[:length]

segment_template = {
        "labelID": "",
        "SegmentDescription": "",
        "SegmentAlgorithmType": "MANUAL",
        "SegmentedPropertyCategoryCodeSequence": {
          "CodeValue": "49755003",
          "CodingSchemeDesignator": "SCT",
          "CodeMeaning": "Morphologically Abnormal Structure"
        },
        "SegmentedPropertyTypeCodeSequence": {
          "CodeValue": "",
          "CodingSchemeDesignator": "99RICORD",
          "CodeMeaning": ""            
        },
        "recommendedDisplayRGBValue": [
          177,
          122,
          101
        ]
      }

    
def convertToNIfTI(series_dir, reconstruction_dir):
#     if nii file exist return it
    logger.debug("Saving reconstruction to ", reconstruction_dir)
    reconstruction = reconstruction_dir+'/ct_image.nii'
    if os.path.exists(reconstruction):
        return reconstruction
    # else create one with dcm2niix
    cmd = ['dcm2niix', '-o', reconstruction_dir, '-f', 'ct_image', series_dir]
    subprocess.run(cmd, stderr=subprocess.DEVNULL, stdout=subprocess.DEVNULL)
    
    return reconstruction
    
def convertToSEG2(input_dicom_dir, seg_dir):

    logger.debug("Saving DICOM SEG to "+seg_dir)
    json_files = glob.glob(seg_dir+"/*.json")
    
    for label_json in json_files:
        
        creator = os.path.split(label_json)[1].split('-')[0]
        label_dcm = os.path.join(seg_dir,creator+".dcm")
        
        labels = glob.glob(seg_dir+'/'+creator+'*.nii')
        labels.sort()
            
        cmd = ['itkimage2segimage','--inputDICOMDirectory',input_dicom_dir,
          '--inputImageList',','.join(labels),"--inputMetadata",
          label_json, "--outputDICOM", label_dcm, "--skip"]
        logger.debug('Running SEG conversion with:\n'+' '.join(cmd))
        result = subprocess.run(cmd, stderr=subprocess.PIPE, stdout=subprocess.PIPE, text=True)
        logger.debug(result.stderr)
        logger.debug(result.stdout)


def main():

  parser = argparse.ArgumentParser(
    usage="%(prog)s --inputDICOM <dir> --inputJSON <name> --outputDirectory <dir>\n\n"
          "Warning: This is an experimenta script in development!\n"
          "The intent of this helper script is to enable conversion"
          "of the MD.ai annotations into appropriate standard DICOM objects.\n")
  parser.add_argument(
    '--inputDICOM',
    dest="inputDICOM",
    default="/Users/fedorov/github/mdai2seg/mydata/mdai_rsna_project_G9qOEdR0_images_2020-10-27-080731",
    metavar='Input directory with the DICOM images being annotated',
    help="Directory with the input DICOM images. It is expected that"
         " the content of this directory is organized into the following"
         "hierarchy: <StudyInstanceUID>/<SeriesInstanceUID>/<SOPInstanceUID>.dcm")
         #,
    #required=True)

  parser.add_argument(
    '--inputJSON',
    dest="inputJSON",
    default="/Users/fedorov/github/mdai2seg/mydata/mdai_rsna_project_G9qOEdR0_annotations_labelgroup_all_2020-10-27-080732.json",
    metavar='Input MD.ai annotations in the native JSON representation',
    help="MD.ai nnotations stored in MD.ai format.")
    #,
    #required=True)

  parser.add_argument(
    '--outputDirectory',
    dest="outputDirectory",
    default="/Users/fedorov/Downloads/test_seg2",
    metavar='Output directory to store the resulting DICOM files',
    help="Directory to store resulting converted DICOM objects.")
    #,
    #required=True)

  parser.add_argument(
    '--includedLabelGroups',
    dest="includedLabelGroups",
    default="\"Team Emily\",\"Team Scott\"",
    metavar='Label groups to consider in conversion',
    help="Only the specified label groups will be considered.")
    #,
    #required=True)  

  args = parser.parse_args()

  if not os.path.exists(args.outputDirectory):
    os.mkdir(args.outputDirectory)

  results = mdai.common_utils.json_to_dataframe(args.inputJSON)  
  annotations_df = results['annotations']
  logger.info(f"{annotations_df.shape[0]} annotations loaded")

  all_series_uids = annotations_df['SeriesInstanceUID'].unique()
  # TODO: Why there are NaNs?
  all_series_uids = all_series_uids[~pd.isnull(all_series_uids)]

  progress_bar = tqdm.tqdm(total=len(all_series_uids))

  for this_series_uid in all_series_uids: 
    #['1.2.826.0.1.3680043.10.474.419639.300423266679936330916265249312']:  # ['1.2.826.0.1.3680043.10.474.440808.1993']: #all_series_uids: # ['1.2.826.0.1.3680043.10.474.2969551981555819856670502082591727602']: 
    logger.debug(f"Processing {this_series_uid}")

    # filter our irrelevant rows
    this_series_annotations = annotations_df[annotations_df["SeriesInstanceUID"] == this_series_uid]
    this_series_annotations = this_series_annotations[this_series_annotations['annotationMode'] == 'freeform']
    this_series_annotations = this_series_annotations[this_series_annotations['groupName'].isin(['Team Emily', 'Team Scott'])]

    if this_series_annotations.shape[0] == 0:
      continue

    logger.debug('Creators of freeform annotations for series '+this_series_uid+" are "+str(this_series_annotations['createdById'].unique()))

    one_row = this_series_annotations.iloc[0]
    
    series_uid = one_row["SeriesInstanceUID"]
    study_path = os.path.join(args.inputDICOM,one_row["StudyInstanceUID"])
    segmentations_path = os.path.join(args.outputDirectory,series_uid+"_SEG")
    reconstruction_path = os.path.join(args.outputDirectory,series_uid+"_Reconstruction")
    
    if not os.path.exists(segmentations_path):
      os.mkdir(segmentations_path)
    if not os.path.exists(reconstruction_path):
      os.mkdir(reconstruction_path)

    series_path = os.path.join(study_path,one_row["SeriesInstanceUID"])
    path_to_instance = os.path.join(series_path,one_row["SOPInstanceUID"]+".dcm")
    instance_dcm = pydicom.dcmread(path_to_instance)

    ct_image_file_name = convertToNIfTI(series_path, reconstruction_path) # run dcm2niix to do this conversion
    PixelType = itk.ctype("signed short")
    image_volume = itk.imread(ct_image_file_name, PixelType)
    image_volume_array = itk.array_view_from_image(image_volume)   
    
    label_images_per_creator = {}
    label_json_per_creator = {}
    creators = this_series_annotations['createdById'].unique()

    for creator in creators:
        label_images_per_creator[creator] = []
        label_json_per_creator[creator] = copy.deepcopy(dcmqi_template)
        label_json_per_creator[creator]['ContentCreatorName'] = creator
        label_json_per_creator[creator]['segmentAttributes'] = []

    for index, row in this_series_annotations.iterrows():
        if row['annotationMode'] != 'freeform':
          progress_bar.update(1)
          continue
        if row['data'] == None or row['data']['vertices'] == None:
          progress_bar.update(1)
          continue

        instance_uid = row['SOPInstanceUID']
        creator = row['createdById']

        path_to_instance = os.path.join(args.inputDICOM,row["StudyInstanceUID"],row["SeriesInstanceUID"],row["SOPInstanceUID"]+".dcm")

        instance_dcm = pydicom.dcmread(path_to_instance) 
        slice_origin_physical_point = instance_dcm.ImagePositionPatient
        slice_origin_index = image_volume.TransformPhysicalPointToIndex(slice_origin_physical_point)
        z = slice_origin_index[2] 

        # make a separate image from single slice
        extractor = itk.ExtractImageFilter.New(image_volume)
        region = image_volume.GetLargestPossibleRegion()
        size = region.GetSize()
        size[2] = 1
        slice_origin_index[0] = 0
        slice_origin_index[1] = 0
        region.SetIndex(slice_origin_index)
        region.SetSize(size)
        extractor.SetExtractionRegion(region)
        extractor.SetInput(image_volume)

        extractor.Update()
        label_slice_image = extractor.GetOutput()
        label_slice_array = itk.array_from_image(label_slice_image)
        label_slice_array.fill(0)

        vertices = np.array(row['data']['vertices']).reshape((-1, 2))
        slice_label_mask = label_slice_array[0,:,:]
        cv2.fillPoly(slice_label_mask, np.int32([vertices]), 1)
        label_slice_array[0,:,:]=np.flipud(slice_label_mask)

        new_image = itk.image_from_array(label_slice_array)
        new_image.SetDirection(label_slice_image.GetDirection())
        new_image.SetOrigin(label_slice_image.TransformIndexToPhysicalPoint(slice_origin_index))
        new_image.SetSpacing(label_slice_image.GetSpacing())

        #logger.debug("added annotation for creator "+creator)

        label_images_per_creator[creator].append(new_image)

        this_segment_template = copy.deepcopy(segment_template)
        this_segment_template['labelID'] = 1
        this_segment_template['SegmentDescription'] = row['labelName']
        this_segment_template['recommendedDisplayRGBValue'] = [int(row['color'][1:3],16), int(row['color'][3:5], 16), int(row['color'][5:7],16)]
        this_segment_template['SegmentedPropertyTypeCodeSequence']['CodeValue'] = makeHash(row['labelName'])
        this_segment_template['SegmentedPropertyTypeCodeSequence']['CodeMeaning'] = row['labelName']
        label_json_per_creator[creator]['segmentAttributes'].append([this_segment_template])
  
    for creator in creators:
      
      logger.debug('Creator '+creator+' has '+str(len(label_images_per_creator[creator]))+' segmentations and '+str(len(label_json_per_creator[creator]['segmentAttributes']))+' seg attrs')
      logger.debug('Processing labels for '+creator)
      for idx, label_itk_image in enumerate(label_images_per_creator[creator]):
        output_file_name = creator+"-"+('%03d' % idx)
        itk.imwrite(label_itk_image, os.path.join(segmentations_path,output_file_name+".nii"))
        
      import json
      output_json_name = creator+"-metadata.json"
      with open(os.path.join(segmentations_path,output_json_name), "w") as json_file:
        json_file.write(json.dumps(label_json_per_creator[creator], indent=2))

    convertToSEG2(series_path, segmentations_path)

    if len(glob.glob(segmentations_path+"/*dcm")) == 0:
      logger.error(f"SEG conversion failed for {segmentations_path}!")

    progress_bar.update(1)

  logger.debug("Done")

  return

if __name__ == "__main__":
  exeFound = {}
  for exe in ['dcm2niix', 'itkimage2segimage']:
    if distutils.spawn.find_executable(exe) is None:
      exeFound[exe] = False
    else:
      exeFound[exe] = True
  if not(exeFound['itkimage2segimage']) or not (exeFound['dcm2niix']):
    logger.error(
      "Dependency converter(s) not found in the path.")
    logger.error(
      "dcmqi (https://github.com/qiicr/dcmqi) and dcm2niix (https://github.com/rordenlab/dcm2niix/releases)")
    logger.error(
      "need to be installed and available in the PATH for using this converter script.")
    sys.exit()

  main()