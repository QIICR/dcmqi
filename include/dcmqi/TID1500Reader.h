#ifndef DCMQI_TID1500READER_H
#define DCMQI_TID1500READER_H

#include "dcmtk/config/osconfig.h"    /* make sure OS specific configuration is included first */

#include "dcmtk/dcmsr/dsrdoc.h"
#include "dcmtk/dcmsr/dsrcodtn.h"
#include "dcmtk/dcmsr/dsrimgtn.h"
#include "dcmtk/dcmsr/dsrnumtn.h"
#include "dcmtk/dcmsr/dsrtextn.h"
#include "dcmtk/dcmsr/dsruidtn.h"
#include "dcmtk/dcmsr/dsrpnmtn.h"

#include "dcmtk/dcmsr/codes/dcm.h"
#include "dcmtk/dcmsr/codes/ncit.h"
#include "dcmtk/dcmsr/codes/sct.h"
#include "dcmtk/dcmsr/codes/srt.h"
#include "dcmtk/dcmsr/codes/umls.h"
#include <dcmtk/dcmsr/dsruidtn.h>

#include "dcmtk/dcmdata/dcfilefo.h"

#include <json/json.h>


// Based on the code provided by @jriesmeier, see
//  https://gist.github.com/fedorov/41e42c1e701d74b2391792241809fe62

class TID1500Reader
  : DSRDocumentTree
{
  public:
    TID1500Reader(const DSRDocumentTree &tree);

    Json::Value getProcedureReported();
    Json::Value getObserverContext();
    Json::Value getMeasurements();
    Json::Value getSingleMeasurement(const DSRNumTreeNode &numNode,
                                     DSRDocumentTreeNodeCursor cursor);
    Json::Value getContentItem(const DSRCodedEntryValue &conceptName,
                               DSRDocumentTreeNodeCursor cursor);

    void initSegmentationContentItems(DSRDocumentTreeNodeCursor cursor, Json::Value &json);

    using DSRDocumentTree::gotoNamedChildNode;

  protected:

    size_t gotoNamedChildNode(const DSRCodedEntryValue &conceptName,
                              DSRDocumentTreeNodeCursor &cursor);
};

#endif // DCMQI_TID1500READER_H
