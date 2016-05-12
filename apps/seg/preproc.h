#ifndef DCMQI_PREPROC_H
#define DCMQI_PREPROC_H

#define CHECK_COND(condition) \
  do { \
    if (condition.bad()) { \
      std::cerr << condition.text() << " in " __FILE__ << ":" << __LINE__  << std::endl; \
      return EXIT_FAILURE; \
    } \
  } while (0)


#endif //DCMQI_PREPROC_H
