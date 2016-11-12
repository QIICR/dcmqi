#ifndef DCMQI_EXCEPTIONS_H
#define DCMQI_EXCEPTIONS_H

// STD includes
#include <map>
#include <stdexcept>
#include <vector>

#define CHECK_COND(condition) \
  do { \
    if (condition.bad()) { \
      std::cerr << "Condition failed: " << condition.text() << " in " __FILE__ << ":" << __LINE__ << std::endl; \
      throw -1; \
    } \
} while (0);

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


  class JSONReadErrorException : public exception {
    virtual const char *what() const throw() {
      return "JSON Exception: file could not be read.";
    }
  };

  class CodeSequenceValueException : public exception {
    virtual const char *what() const throw() {
      return "CodeSequence Exception: missing value in code sequence";
    }
  };
}

#endif //DCMQI_EXCEPTIONS_H
