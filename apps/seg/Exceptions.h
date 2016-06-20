#ifndef DCMQI_EXCEPTIONS_H
#define DCMQI_EXCEPTIONS_H

#include <vector>
#include <map>

using namespace std;

namespace dcmqi {

    class OFConditionBadException : public exception {
        virtual const char *what() const throw() {
            return "DICOM Exception: ";
        }
    };

    class DCMQIImagePositionPatientMapsOutsideITKException : public runtime_error {
    public:
        DCMQIImagePositionPatientMapsOutsideITKException(const char* m) : std::runtime_error(m) { }
    };
}

#endif //DCMQI_EXCEPTIONS_H
