// Entry point of a dcmqi command-line application built by dcmqiMacroBuildCLI.
//
// The application's own main() is compiled into the "<module>Lib" library
// under the name ModuleEntryPoint, so that the test drivers can link the
// application code without a second main().

int ModuleEntryPoint(int argc, char* argv[]);

int main(int argc, char* argv[])
{
  return ModuleEntryPoint(argc, argv);
}
