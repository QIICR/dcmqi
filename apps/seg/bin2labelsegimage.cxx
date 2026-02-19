// CLP includes
#include "bin2labelsegimageCLP.h"
#include <itkSmartPointer.h>

// DCMQI includes
#undef HAVE_SSTREAM // Avoid redefinition warning
#include "dcmqi/Bin2Label.h"
#include "dcmqi/internal/VersionConfigure.h"

// DCMTK includes
#include <dcmtk/oflog/configrt.h>
#include <dcmtk/dcmdata/dcrledrg.h>
#include <dcmtk/dcmdata/dcrleerg.h>

typedef dcmqi::Helper helper;
typedef itk::ImageFileWriter<ShortImageType> WriterType;

int main(int argc, char* argv[])
{
    std::cout << dcmqi_INFO << std::endl;

    PARSE_ARGS;
    E_TransferSyntax outputTS = EXS_RLELossless;

    if (verbose)
    {
        // Display DCMTK debug, warning, and error logs in the console
        dcmtk::log4cplus::BasicConfigurator::doConfigure();
    }

    if (helper::isUndefinedOrPathDoesNotExist(inputSEGFileName, "Input DICOM file"))
        return EXIT_FAILURE;


    DcmRLEDecoderRegistration::registerCodecs();
    DcmRLEEncoderRegistration::registerCodecs();

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
        if (compressRLE)
        {
            outputTS = EXS_RLELossless;
        }
#ifdef WITH_ZLIB
        else if (compressZLIB)
        {
            outputTS = EXS_DeflatedLittleEndianExplicit;
        }
#else
        else if (compressZLIB)
        {
            std::cerr << "ERROR: ZLIB compression is not supported because DCMTK was built without ZLIB support." << std::endl;
            return EXIT_FAILURE;
        }
#endif
        else if (compressNone)
        {
            outputTS = EXS_LittleEndianExplicit;
        }
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
                // choose representation
                result = outputFF.chooseRepresentation(outputTS, NULL);
                if (result.good())
                {
                    std::cout << "Using transfer syntax: " << DcmXfer(outputTS).getXferName() << std::endl;
                    CHECK_COND(outputFF.saveFile(outputSEGFileName.c_str(), outputTS));
                }
                else
                {
                    std::cerr << "ERROR: Failed to convert to desired output transfer syntax." << std::endl;
                    returnCode = EXIT_FAILURE;
                }
            }
            else
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
    DcmRLEEncoderRegistration::cleanup();
    return returnCode;
}
