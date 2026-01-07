// CLP includes
#include "bin2labelsegimageCLP.h"
#include <itkSmartPointer.h>

// DCMQI includes
#undef HAVE_SSTREAM // Avoid redefinition warning
#include "dcmqi/Bin2Label.h"
#include "dcmqi/internal/VersionConfigure.h"

// DCMTK includes
#include <dcmtk/oflog/configrt.h>

typedef dcmqi::Helper helper;
typedef itk::ImageFileWriter<ShortImageType> WriterType;

int main(int argc, char* argv[])
{
    std::cout << dcmqi_INFO << std::endl;

    PARSE_ARGS;

    if (verbose)
    {
        // Display DCMTK debug, warning, and error logs in the console
        dcmtk::log4cplus::BasicConfigurator::doConfigure();
    }

    if (helper::isUndefinedOrPathDoesNotExist(inputSEGFileName, "Input DICOM file"))
        return EXIT_FAILURE;


    DcmRLEDecoderRegistration::registerCodecs();

    DcmFileFormat sliceFF;
    std::cout << "Loading DICOM SEG file " << inputSEGFileName << std::endl;
    CHECK_COND(sliceFF.loadFile(inputSEGFileName.c_str()));
    DcmDataset* dataset = sliceFF.getDataset();
    int returnCode = EXIT_SUCCESS;
    try
    {
        dcmqi::DcmBinToLabelConverter converter;
        std::string metaInfo;
        dcmqi::DcmBinToLabelConverter::ConversionFlags convFlags;
        if (usePalette)
        {
            convFlags.m_outputColorModel = DcmSegTypes::SLCM_PALETTE;
            convFlags.m_forcePalette     = OFTrue;
        }
        else
            convFlags.m_outputColorModel = DcmSegTypes::SLCM_MONOCHROME2;

        if (noCheck)
        {
            convFlags.m_checkExportFG     = OFFalse;
            convFlags.m_checkExportValues = OFFalse;
        }
        converter.setInput(dataset);
        std::cout << "Converting binary segmentation to label map segmentation..." << std::endl;
        OFCondition result = converter.convert(convFlags);
        if (result.good())
        {
            OFshared_ptr<DcmSegmentation> labelSeg;
            result = converter.getOutputSegmentation(labelSeg);
            if (result.good() && labelSeg)
            {
                std::cout << "Successfully converted binary segmentation to label map segmentation." << std::endl;
                // Now, write out the resulting label map segmentation
                DcmFileFormat outputFF;
                if (!convFlags.m_checkExportFG)
                {
                  labelSeg->getFunctionalGroups().setCheckOnWrite(OFFalse);
                }
                if (!convFlags.m_checkExportValues)
                {
                  labelSeg->setValueCheckOnWrite(OFFalse);
                }

                CHECK_COND(labelSeg->writeDataset(*(outputFF.getDataset())));
                std::cout << "Writing output DICOM label map SEG file to " << outputSEGFileName << std::endl;
                CHECK_COND(outputFF.saveFile(outputSEGFileName.c_str(), EXS_LittleEndianExplicit));
            }
            else
            {
                std::cerr << "ERROR: Failed to get output label segmentation: " << result.text() << std::endl;
                returnCode = EXIT_FAILURE;
            }
            if (result.bad() || !labelSeg)
            {
                std::cerr << "ERROR: Failed to get output label segmentation: " << result.text() << std::endl;
                returnCode = EXIT_FAILURE;
            }
        }
        else
        {
            std::cerr << "ERROR: Failed to convert DICOM binary SEG to DICOM labelmap SEG: " << result.text()
                      << std::endl;
            returnCode = EXIT_FAILURE;
        }
    }
    catch (int e)
    {
        std::cerr << "Fatal error encountered." << std::endl;
        returnCode = EXIT_FAILURE;
    }

    // deregister RLE codec
    DcmRLEDecoderRegistration::cleanup();
    return returnCode;
}
