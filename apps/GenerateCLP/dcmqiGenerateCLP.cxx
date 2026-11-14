/*
 * dcmqiGenerateCLP
 *
 * Build-time tool that reads the XML descriptor of a dcmqi command-line
 * application and writes the "<module>CLP.h" header that provides the
 * PARSE_ARGS macro used at the top of the application's main().
 *
 *   dcmqiGenerateCLP <module>.xml <output-header>
 *
 * PARSE_ARGS declares one local variable per <parameters> entry, named after
 * its <name> and initialized from <default>, and fills them from the command
 * line using TCLAP. Calling the program with "--xml" as its first argument
 * prints the descriptor and exits.
 *
 * Only the part of the descriptor schema used by dcmqi is accepted. Anything
 * else is reported as an error, so that a change to a descriptor cannot
 * silently change the command-line interface.
 *
 * The tool depends on the C++ standard library only, so that it runs during
 * the build without any shared library on the search path.
 */

#include <cctype>
#include <cerrno>
#include <climits>
#include <cstdio>
#include <cstdlib>
#include <fstream>
#include <iostream>
#include <iterator>
#include <set>
#include <sstream>
#include <stdexcept>
#include <string>
#include <utility>
#include <vector>

namespace
{

class DescriptorError : public std::runtime_error
{
public:
  explicit DescriptorError(const std::string& message)
    : std::runtime_error(message)
  {
  }
};

//----------------------------------------------------------------------------
// XML reading
//
// The descriptors are small files written by hand, so the reader supports
// what they need and rejects the rest: an optional XML declaration, comments,
// elements, attributes, character data, and the predefined and numeric
// character references. DOCTYPE, CDATA sections and processing instructions
// are errors.

struct Element
{
  std::string name;
  int line;
  std::vector<std::pair<std::string, std::string> > attributes;
  std::vector<Element> children;
  // Character data directly inside the element, with comments removed and
  // line ends normalized to "\n".
  std::string text;

  Element()
    : line(0)
  {
  }
};

std::string LineContext(int line)
{
  std::ostringstream context;
  context << "line " << line;
  return context.str();
}

// XML end-of-line handling: "\r\n" and "\r" both become "\n".
std::string NormalizeLineEnds(const std::string& text)
{
  std::string result;
  result.reserve(text.size());
  for (std::string::size_type i = 0; i < text.size(); ++i)
  {
    if (text[i] == '\r')
    {
      result += '\n';
      if (i + 1 < text.size() && text[i + 1] == '\n')
      {
        ++i;
      }
    }
    else
    {
      result += text[i];
    }
  }
  return result;
}

class XMLReader
{
public:
  explicit XMLReader(const std::string& document)
    : Text(NormalizeLineEnds(document))
    , Position(0)
    , Line(1)
  {
  }

  Element ReadDocument()
  {
    if (this->LookingAt("\xEF\xBB\xBF"))
    {
      this->Advance(3);
    }
    if (this->LookingAt("<?xml") && this->Position + 5 < this->Text.size()
        && (std::isspace(static_cast<unsigned char>(this->Text[this->Position + 5]))
            || this->Text[this->Position + 5] == '?'))
    {
      const std::string::size_type end = this->Text.find("?>", this->Position);
      if (end == std::string::npos)
      {
        this->Fail("unterminated XML declaration");
      }
      this->Advance(end + 2 - this->Position);
    }
    this->SkipMisc();
    if (!this->LookingAt("<") || this->LookingAt("<!") || this->LookingAt("<?"))
    {
      this->Fail("expected the root element (DOCTYPE and processing instructions are not supported)");
    }
    const Element root = this->ReadElement();
    this->SkipMisc();
    if (this->Position != this->Text.size())
    {
      this->Fail("unexpected content after the root element");
    }
    return root;
  }

private:
  const std::string Text;
  std::string::size_type Position;
  int Line;

  [[noreturn]] void Fail(const std::string& message) const
  {
    throw DescriptorError(LineContext(this->Line) + ": " + message);
  }

  bool LookingAt(const char* prefix) const
  {
    return this->Text.compare(this->Position, std::char_traits<char>::length(prefix), prefix) == 0;
  }

  void Advance(std::string::size_type count)
  {
    for (std::string::size_type i = 0; i < count && this->Position < this->Text.size(); ++i)
    {
      if (this->Text[this->Position++] == '\n')
      {
        ++this->Line;
      }
    }
  }

  void Expect(const char* token)
  {
    if (!this->LookingAt(token))
    {
      this->Fail(std::string("expected '") + token + "'");
    }
    this->Advance(std::char_traits<char>::length(token));
  }

  bool SkipWhitespace()
  {
    const std::string::size_type start = this->Position;
    while (this->Position < this->Text.size()
           && (this->Text[this->Position] == ' ' || this->Text[this->Position] == '\t'
               || this->Text[this->Position] == '\n'))
    {
      this->Advance(1);
    }
    return this->Position != start;
  }

  void SkipComment()
  {
    const std::string::size_type end = this->Text.find("-->", this->Position + 4);
    if (end == std::string::npos)
    {
      this->Fail("unterminated comment");
    }
    this->Advance(end + 3 - this->Position);
  }

  // Whitespace and comments outside the root element.
  void SkipMisc()
  {
    for (;;)
    {
      this->SkipWhitespace();
      if (!this->LookingAt("<!--"))
      {
        return;
      }
      this->SkipComment();
    }
  }

  std::string ReadName()
  {
    const std::string::size_type start = this->Position;
    while (this->Position < this->Text.size())
    {
      const unsigned char c = static_cast<unsigned char>(this->Text[this->Position]);
      const bool first = this->Position == start;
      if (!(std::isalpha(c) || c == '_' || c == ':' || (!first && (std::isdigit(c) || c == '-' || c == '.'))))
      {
        break;
      }
      this->Advance(1);
    }
    if (this->Position == start)
    {
      this->Fail("expected a name");
    }
    return this->Text.substr(start, this->Position - start);
  }

  static void AppendUTF8(unsigned long code, std::string& out)
  {
    if (code < 0x80)
    {
      out += static_cast<char>(code);
    }
    else if (code < 0x800)
    {
      out += static_cast<char>(0xC0 | (code >> 6));
      out += static_cast<char>(0x80 | (code & 0x3F));
    }
    else if (code < 0x10000)
    {
      out += static_cast<char>(0xE0 | (code >> 12));
      out += static_cast<char>(0x80 | ((code >> 6) & 0x3F));
      out += static_cast<char>(0x80 | (code & 0x3F));
    }
    else
    {
      out += static_cast<char>(0xF0 | (code >> 18));
      out += static_cast<char>(0x80 | ((code >> 12) & 0x3F));
      out += static_cast<char>(0x80 | ((code >> 6) & 0x3F));
      out += static_cast<char>(0x80 | (code & 0x3F));
    }
  }

  // Reads "&...;" and returns the text it stands for.
  std::string ReadReference()
  {
    this->Expect("&");
    const std::string::size_type end = this->Text.find(';', this->Position);
    if (end == std::string::npos || end == this->Position || end - this->Position > 10)
    {
      this->Fail("malformed reference");
    }
    const std::string name = this->Text.substr(this->Position, end - this->Position);
    this->Advance(name.size() + 1);

    if (name == "lt")
      return "<";
    if (name == "gt")
      return ">";
    if (name == "amp")
      return "&";
    if (name == "quot")
      return "\"";
    if (name == "apos")
      return "'";
    if (name[0] == '#')
    {
      const bool hex = name.size() > 1 && name[1] == 'x';
      const std::string digits = name.substr(hex ? 2 : 1);
      char* digitsEnd = nullptr;
      const unsigned long code = std::strtoul(digits.c_str(), &digitsEnd, hex ? 16 : 10);
      if (!digits.empty() && std::isxdigit(static_cast<unsigned char>(digits[0])) && *digitsEnd == '\0'
          && code > 0 && code <= 0x10FFFF && !(code >= 0xD800 && code <= 0xDFFF))
      {
        std::string text;
        AppendUTF8(code, text);
        return text;
      }
    }
    this->Fail("unsupported reference '&" + name + ";'");
  }

  std::string ReadAttributeValue()
  {
    if (!this->LookingAt("\"") && !this->LookingAt("'"))
    {
      this->Fail("expected a quoted attribute value");
    }
    const char quote = this->Text[this->Position];
    this->Advance(1);
    std::string value;
    for (;;)
    {
      if (this->Position == this->Text.size() || this->Text[this->Position] == '<')
      {
        this->Fail("unterminated attribute value");
      }
      if (this->Text[this->Position] == quote)
      {
        this->Advance(1);
        return value;
      }
      if (this->Text[this->Position] == '&')
      {
        value += this->ReadReference();
      }
      else
      {
        value += this->Text[this->Position];
        this->Advance(1);
      }
    }
  }

  Element ReadElement()
  {
    Element element;
    element.line = this->Line;
    this->Expect("<");
    element.name = this->ReadName();

    // Attributes
    for (;;)
    {
      const bool separated = this->SkipWhitespace();
      if (this->LookingAt("/>"))
      {
        this->Advance(2);
        return element;
      }
      if (this->LookingAt(">"))
      {
        this->Advance(1);
        break;
      }
      if (!separated)
      {
        this->Fail("expected whitespace, '>' or '/>' in <" + element.name + ">");
      }
      const std::string name = this->ReadName();
      this->SkipWhitespace();
      this->Expect("=");
      this->SkipWhitespace();
      const std::string value = this->ReadAttributeValue();
      for (std::size_t i = 0; i < element.attributes.size(); ++i)
      {
        if (element.attributes[i].first == name)
        {
          this->Fail("duplicate attribute '" + name + "'");
        }
      }
      element.attributes.push_back(std::make_pair(name, value));
    }

    // Content
    for (;;)
    {
      if (this->Position == this->Text.size())
      {
        this->Fail("<" + element.name + "> (line " + std::to_string(element.line) + ") is not closed");
      }
      if (this->LookingAt("</"))
      {
        this->Advance(2);
        const std::string name = this->ReadName();
        if (name != element.name)
        {
          this->Fail("</" + name + "> does not close <" + element.name + "> (line " + std::to_string(element.line)
                     + ")");
        }
        this->SkipWhitespace();
        this->Expect(">");
        return element;
      }
      if (this->LookingAt("<!--"))
      {
        this->SkipComment();
      }
      else if (this->LookingAt("<!") || this->LookingAt("<?"))
      {
        this->Fail("CDATA sections and processing instructions are not supported");
      }
      else if (this->LookingAt("<"))
      {
        element.children.push_back(this->ReadElement());
      }
      else if (this->LookingAt("&"))
      {
        element.text += this->ReadReference();
      }
      else
      {
        element.text += this->Text[this->Position];
        this->Advance(1);
      }
    }
  }
};

//----------------------------------------------------------------------------
// Descriptor model

enum ParameterType
{
  FileParameter,
  DirectoryParameter,
  StringParameter,
  EnumerationParameter,
  IntegerParameter,
  BooleanParameter,
  StringVectorParameter
};

struct ParameterKind
{
  const char* tag;
  ParameterType type;
};

const ParameterKind ParameterKinds[] = {
  { "file", FileParameter },
  { "directory", DirectoryParameter },
  { "string", StringParameter },
  { "string-enumeration", EnumerationParameter },
  { "integer", IntegerParameter },
  { "boolean", BooleanParameter },
  { "string-vector", StringVectorParameter },
};

struct Parameter
{
  ParameterType type;
  std::string context; // for error messages
  std::string name;
  std::string flag;     // without the leading '-'
  std::string longflag; // without the leading "--"
  std::string description;
  bool hasDefault;
  std::string defaultValue;
  std::vector<std::string> elements;

  Parameter()
    : type(StringParameter)
    , hasDefault(false)
  {
  }
};

struct Module
{
  std::string name;
  std::string description;
  std::string version;
  std::string contributor;
  std::string acknowledgements;
  std::vector<Parameter> parameters;
};

// Names the generated code uses besides the module's own parameters.
const char* const ReservedNames[] = {
  "echoSwitch", "xmlSwitch", "processInformationAddressString", "returnParameterFile", "argc", "argv",
};
const char* const ReservedLongFlags[] = {
  "help", "version", "ignore_rest", "echo", "xml", "processinformationaddress", "returnparameterfile",
};

std::string Trim(const std::string& text)
{
  const char* const whitespace = " \t\r\n";
  const std::string::size_type begin = text.find_first_not_of(whitespace);
  if (begin == std::string::npos)
  {
    return std::string();
  }
  const std::string::size_type end = text.find_last_not_of(whitespace);
  return text.substr(begin, end - begin + 1);
}

std::string StripLeadingDashes(const std::string& text)
{
  const std::string::size_type begin = text.find_first_not_of('-');
  return begin == std::string::npos ? std::string() : text.substr(begin);
}

bool IsIdentifier(const std::string& text)
{
  if (text.empty() || !(std::isalpha(static_cast<unsigned char>(text[0])) || text[0] == '_'))
  {
    return false;
  }
  for (std::string::size_type i = 1; i < text.size(); ++i)
  {
    if (!(std::isalnum(static_cast<unsigned char>(text[i])) || text[i] == '_'))
    {
      return false;
    }
  }
  return true;
}

bool IsLongFlag(const std::string& text)
{
  if (text.empty() || !std::isalnum(static_cast<unsigned char>(text[0])))
  {
    return false;
  }
  for (std::string::size_type i = 1; i < text.size(); ++i)
  {
    const unsigned char c = static_cast<unsigned char>(text[i]);
    if (!(std::isalnum(c) || c == '_' || c == '-'))
    {
      return false;
    }
  }
  return true;
}

template <std::size_t N>
bool Contains(const char* const (&list)[N], const std::string& value)
{
  for (std::size_t i = 0; i < N; ++i)
  {
    if (value == list[i])
    {
      return true;
    }
  }
  return false;
}

std::string ElementContext(const Element& element)
{
  return LineContext(element.line) + ", <" + element.name + ">";
}

// An element that may only contain other elements (and whitespace).
void CheckNoText(const Element& element)
{
  if (!Trim(element.text).empty())
  {
    throw DescriptorError(ElementContext(element) + ": unexpected text '" + Trim(element.text) + "'");
  }
}

// The text of an element that may only contain text.
std::string ElementText(const Element& element)
{
  if (!element.children.empty())
  {
    throw DescriptorError(ElementContext(element.children[0]) + ": not allowed inside <" + element.name + ">");
  }
  if (!element.attributes.empty())
  {
    throw DescriptorError(ElementContext(element) + ": unsupported attribute '" + element.attributes[0].first + "'");
  }
  return Trim(element.text);
}

Parameter ReadParameter(const Element& element, ParameterType type)
{
  Parameter parameter;
  parameter.type = type;
  parameter.context = ElementContext(element);

  if (!element.attributes.empty())
  {
    throw DescriptorError(parameter.context + ": unsupported attribute '" + element.attributes[0].first + "'");
  }
  CheckNoText(element);

  std::set<std::string> seen;
  for (std::size_t i = 0; i < element.children.size(); ++i)
  {
    const Element& child = element.children[i];
    const std::string& tag = child.name;
    const std::string text = ElementText(child);

    if (tag == "element" && type == EnumerationParameter)
    {
      parameter.elements.push_back(text);
      continue;
    }
    if (!seen.insert(tag).second)
    {
      throw DescriptorError(ElementContext(child) + ": more than one <" + tag + "> in <" + element.name + ">");
    }
    if (tag == "name")
    {
      parameter.name = text;
    }
    else if (tag == "flag")
    {
      parameter.flag = StripLeadingDashes(text);
    }
    else if (tag == "longflag")
    {
      parameter.longflag = StripLeadingDashes(text);
    }
    else if (tag == "description")
    {
      parameter.description = text;
    }
    else if (tag == "default")
    {
      parameter.hasDefault = true;
      parameter.defaultValue = text;
    }
    else if (tag == "channel")
    {
      if (text != "input" && text != "output")
      {
        throw DescriptorError(ElementContext(child) + ": unsupported value '" + text + "'");
      }
    }
    else if (tag != "label")
    {
      throw DescriptorError(ElementContext(child) + ": unsupported element in <" + element.name + ">");
    }
  }

  const std::string& context = parameter.context;
  if (!IsIdentifier(parameter.name))
  {
    throw DescriptorError(context + ": <name> must be a C++ identifier");
  }
  if (Contains(ReservedNames, parameter.name) || parameter.name.compare(0, 8, "dcmqiCLP") == 0)
  {
    throw DescriptorError(context + ": <name> '" + parameter.name + "' is reserved");
  }
  if (!IsLongFlag(parameter.longflag))
  {
    throw DescriptorError(context + ": a <longflag> made of letters, digits, '_' and '-' is required");
  }
  if (Contains(ReservedLongFlags, parameter.longflag))
  {
    throw DescriptorError(context + ": <longflag> '" + parameter.longflag + "' is reserved");
  }
  if (seen.count("flag")
      && (parameter.flag.size() != 1 || !std::isalnum(static_cast<unsigned char>(parameter.flag[0]))
          || parameter.flag == "h"))
  {
    throw DescriptorError(context + ": <flag> must be a single letter or digit other than 'h'");
  }

  switch (type)
  {
    case IntegerParameter:
      if (parameter.hasDefault)
      {
        const char* begin = parameter.defaultValue.c_str();
        char* end = nullptr;
        errno = 0;
        const long value = std::strtol(begin, &end, 10);
        if (parameter.defaultValue.empty() || *end != '\0' || errno == ERANGE || value < INT_MIN
            || value > INT_MAX)
        {
          throw DescriptorError(context + ": <default> must be an integer");
        }
      }
      break;
    case BooleanParameter:
      // A switch can only turn an option on, so the default must be off.
      if (parameter.hasDefault && parameter.defaultValue != "false")
      {
        throw DescriptorError(context + ": <default> of a boolean must be 'false'");
      }
      break;
    case EnumerationParameter:
      if (parameter.elements.empty())
      {
        throw DescriptorError(context + ": at least one <element> is required");
      }
      if (parameter.hasDefault && !parameter.defaultValue.empty())
      {
        bool found = false;
        for (std::size_t i = 0; i < parameter.elements.size(); ++i)
        {
          found = found || parameter.elements[i] == parameter.defaultValue;
        }
        if (!found)
        {
          throw DescriptorError(context + ": <default> is not one of the <element> values");
        }
      }
      break;
    default:
      break;
  }
  return parameter;
}

void ReadParameterGroup(const Element& group, Module& module)
{
  for (std::size_t i = 0; i < group.attributes.size(); ++i)
  {
    const std::string& name = group.attributes[i].first;
    const std::string& value = group.attributes[i].second;
    if (name != "advanced" || (value != "true" && value != "false"))
    {
      throw DescriptorError(ElementContext(group) + ": unsupported attribute " + name + "=\"" + value + "\"");
    }
  }
  CheckNoText(group);
  for (std::size_t i = 0; i < group.children.size(); ++i)
  {
    const Element& child = group.children[i];
    if (child.name == "label" || child.name == "description")
    {
      ElementText(child);
      continue;
    }
    bool known = false;
    for (std::size_t k = 0; k < sizeof(ParameterKinds) / sizeof(ParameterKinds[0]); ++k)
    {
      if (child.name == ParameterKinds[k].tag)
      {
        module.parameters.push_back(ReadParameter(child, ParameterKinds[k].type));
        known = true;
        break;
      }
    }
    if (!known)
    {
      throw DescriptorError(ElementContext(child) + ": unsupported element in <parameters>");
    }
  }
}

Module ReadDescriptor(const std::string& xml)
{
  XMLReader reader(xml);
  const Element root = reader.ReadDocument();
  if (root.name != "executable")
  {
    throw DescriptorError(ElementContext(root) + ": the root element must be <executable>");
  }
  if (!root.attributes.empty())
  {
    throw DescriptorError(ElementContext(root) + ": unsupported attribute '" + root.attributes[0].first + "'");
  }
  CheckNoText(root);

  Module module;
  std::set<std::string> seen;
  for (std::size_t i = 0; i < root.children.size(); ++i)
  {
    const Element& child = root.children[i];
    const std::string& tag = child.name;
    if (tag == "parameters")
    {
      ReadParameterGroup(child, module);
      continue;
    }
    if (tag != "category" && tag != "title" && tag != "description" && tag != "version"
        && tag != "documentation-url" && tag != "license" && tag != "contributor" && tag != "acknowledgements")
    {
      throw DescriptorError(ElementContext(child) + ": unsupported element in <executable>");
    }
    if (!seen.insert(tag).second)
    {
      throw DescriptorError(ElementContext(child) + ": more than one <" + tag + "> in <executable>");
    }
    const std::string text = ElementText(child);
    if (tag == "description")
    {
      module.description = text;
    }
    else if (tag == "version")
    {
      module.version = text;
    }
    else if (tag == "contributor")
    {
      module.contributor = text;
    }
    else if (tag == "acknowledgements")
    {
      module.acknowledgements = text;
    }
  }

  std::set<std::string> names;
  std::set<std::string> flags;
  std::set<std::string> longflags;
  for (std::size_t i = 0; i < module.parameters.size(); ++i)
  {
    const Parameter& parameter = module.parameters[i];
    if (!names.insert(parameter.name).second)
    {
      throw DescriptorError(parameter.context + ": <name> '" + parameter.name + "' is used more than once");
    }
    if (!longflags.insert(parameter.longflag).second)
    {
      throw DescriptorError(parameter.context + ": <longflag> '" + parameter.longflag + "' is used more than once");
    }
    if (!parameter.flag.empty() && !flags.insert(parameter.flag).second)
    {
      throw DescriptorError(parameter.context + ": <flag> '" + parameter.flag + "' is used more than once");
    }
  }
  return module;
}

//----------------------------------------------------------------------------
// Header writing

// C++ string literal for arbitrary bytes.
std::string Literal(const std::string& text)
{
  std::string result = "\"";
  for (std::string::size_type i = 0; i < text.size(); ++i)
  {
    const unsigned char c = static_cast<unsigned char>(text[i]);
    switch (c)
    {
      case '\\':
        result += "\\\\";
        break;
      case '"':
        result += "\\\"";
        break;
      case '?': // avoid trigraphs
        result += "\\?";
        break;
      case '\n':
        result += "\\n";
        break;
      case '\t':
        result += "\\t";
        break;
      default:
        if (c < 0x20 || c > 0x7e)
        {
          char escaped[8];
          std::snprintf(escaped, sizeof(escaped), "\\%03o", static_cast<unsigned int>(c));
          result += escaped;
        }
        else
        {
          result += static_cast<char>(c);
        }
    }
  }
  return result + "\"";
}

std::string CppType(ParameterType type)
{
  switch (type)
  {
    case IntegerParameter:
      return "int";
    case BooleanParameter:
      return "bool";
    case StringVectorParameter:
      return "std::vector<std::string>";
    default:
      return "std::string";
  }
}

// Value shown in the usage message as " (value: ...)".
std::string DisplayedDefault(const Parameter& parameter)
{
  if (parameter.type == BooleanParameter)
  {
    return parameter.hasDefault ? "0" : "";
  }
  return parameter.defaultValue;
}

std::string HelpText(const Parameter& parameter)
{
  const std::string shown = DisplayedDefault(parameter);
  return shown.empty() ? parameter.description : parameter.description + " (value: " + shown + ")";
}

// Splits text into pieces that each end with a newline (the last one may
// not), with no piece longer than 'maximum' bytes, so that no string literal
// in the generated header gets too long for the compiler.
std::vector<std::string> SplitLines(const std::string& text, std::string::size_type maximum)
{
  std::vector<std::string> pieces;
  std::string::size_type start = 0;
  while (start < text.size())
  {
    std::string::size_type end = text.find('\n', start);
    end = (end == std::string::npos) ? text.size() : end + 1;
    if (end - start > maximum)
    {
      end = start + maximum;
    }
    pieces.push_back(text.substr(start, end - start));
    start = end;
  }
  return pieces;
}

void WriteHeader(std::ostream& out, const Module& module, const std::string& xmlFileName, const std::string& xml)
{
  const std::string ns = module.name + "CLP";
  const std::vector<Parameter>& parameters = module.parameters;

  std::string fullDescription = "Description: " + module.description;
  if (!module.contributor.empty())
  {
    fullDescription += "\nAuthor(s): " + module.contributor;
  }
  if (!module.acknowledgements.empty())
  {
    fullDescription += "\nAcknowledgements: " + module.acknowledgements;
  }

  out << "// Generated by dcmqiGenerateCLP from " << xmlFileName << ". Do not edit.\n"
      << "\n"
      << "#ifndef " << ns << "_h\n"
      << "#define " << ns << "_h\n"
      << "\n"
      << "#include \"dcmqiCLP.h\"\n"
      << "\n"
      << "namespace " << ns << "\n"
      << "{\n"
      << "\n"
      << "/// The XML descriptor of this module, printed by --xml.\n"
      << "inline std::string XMLDescription()\n"
      << "{\n"
      << "  static const char* const lines[] = {\n";
  const std::vector<std::string> lines = SplitLines(xml, 1024);
  for (std::size_t i = 0; i < lines.size(); ++i)
  {
    out << "    " << Literal(lines[i]) << ",\n";
  }
  if (lines.empty())
  {
    out << "    \"\",\n";
  }
  out << "  };\n"
      << "  return dcmqiCLP::JoinLines(lines, sizeof(lines) / sizeof(lines[0]));\n"
      << "}\n"
      << "\n"
      << "struct Arguments\n"
      << "{\n";
  for (std::size_t i = 0; i < parameters.size(); ++i)
  {
    const Parameter& p = parameters[i];
    out << "  " << CppType(p.type) << " " << p.name;
    if (p.type == IntegerParameter)
    {
      out << " = " << (p.hasDefault ? p.defaultValue : "0");
    }
    else if (p.type == BooleanParameter)
    {
      out << " = false";
    }
    out << ";\n";
  }
  out << "  bool echoSwitch = false;\n"
      << "  bool xmlSwitch = false;\n"
      << "  std::string processInformationAddressString;\n"
      << "  std::string returnParameterFile;\n"
      << "};\n"
      << "\n"
      << "/// Fills 'arguments' from the command line. Returns false when the program\n"
      << "/// must exit right away with 'exitStatus'.\n"
      << "inline bool ParseArguments(int argc, char* argv[], Arguments& arguments, int& exitStatus)\n"
      << "{\n"
      << "  if (argc > 1 && std::string(argv[1]) == \"--xml\")\n"
      << "  {\n"
      << "    std::cout << XMLDescription() << std::endl;\n"
      << "    exitStatus = EXIT_SUCCESS;\n"
      << "    return false;\n"
      << "  }\n"
      << "\n"
      << "  try\n"
      << "  {\n"
      << "    TCLAP::CmdLine commandLine(\n"
      << "      " << Literal(fullDescription) << ",\n"
      << "      ' ', " << Literal(module.version) << ");\n"
      << "\n";

  for (std::size_t i = 0; i < parameters.size(); ++i)
  {
    const Parameter& p = parameters[i];
    const std::string flag = Literal(p.flag);
    const std::string longflag = Literal(p.longflag);
    const std::string help = Literal(HelpText(p));
    switch (p.type)
    {
      case BooleanParameter:
        out << "    TCLAP::SwitchArg " << p.name << "Arg(" << flag << ", " << longflag << ",\n"
            << "      " << help << ",\n"
            << "      commandLine, false);\n";
        break;
      case EnumerationParameter:
        out << "    std::vector<std::string> " << p.name << "Allowed;\n";
        for (std::size_t k = 0; k < p.elements.size(); ++k)
        {
          out << "    " << p.name << "Allowed.push_back(" << Literal(p.elements[k]) << ");\n";
        }
        out << "    TCLAP::ValuesConstraint<std::string> " << p.name << "Constraint(" << p.name << "Allowed);\n"
            << "    TCLAP::ValueArg<std::string> " << p.name << "Arg(" << flag << ", " << longflag << ",\n"
            << "      " << help << ",\n"
            << "      false, " << Literal(p.defaultValue) << ", &" << p.name << "Constraint, commandLine);\n";
        break;
      case IntegerParameter:
        out << "    TCLAP::ValueArg<int> " << p.name << "Arg(" << flag << ", " << longflag << ",\n"
            << "      " << help << ",\n"
            << "      false, " << (p.hasDefault ? p.defaultValue : "0") << ", \"int\", commandLine);\n";
        break;
      case StringVectorParameter:
        out << "    TCLAP::ValueArg<std::string> " << p.name << "Arg(" << flag << ", " << longflag << ",\n"
            << "      " << help << ",\n"
            << "      false, " << Literal(p.defaultValue) << ", \"std::vector<std::string>\", commandLine);\n";
        break;
      default:
        out << "    TCLAP::ValueArg<std::string> " << p.name << "Arg(" << flag << ", " << longflag << ",\n"
            << "      " << help << ",\n"
            << "      false, " << Literal(p.defaultValue) << ", \"std::string\", commandLine);\n";
        break;
    }
  }

  out << "    dcmqiCLP::StandardArguments standardArguments(commandLine);\n"
      << "\n"
      << "    std::set<std::string> valueFlags;\n"
      << "    dcmqiCLP::StandardArguments::AddValueFlags(valueFlags);\n";
  for (std::size_t i = 0; i < parameters.size(); ++i)
  {
    const Parameter& p = parameters[i];
    if (p.type == BooleanParameter)
    {
      continue;
    }
    out << "    valueFlags.insert(" << Literal("--" + p.longflag) << ");\n";
    if (!p.flag.empty())
    {
      out << "    valueFlags.insert(" << Literal("-" + p.flag) << ");\n";
    }
  }
  out << "\n"
      << "    std::vector<std::string> tokens = dcmqiCLP::NormalizeArguments(argc, argv, valueFlags);\n"
      << "    commandLine.parse(tokens);\n"
      << "\n";
  for (std::size_t i = 0; i < parameters.size(); ++i)
  {
    const Parameter& p = parameters[i];
    if (p.type == StringVectorParameter)
    {
      out << "    arguments." << p.name << " = dcmqiCLP::SplitList(" << p.name << "Arg.getValue());\n";
    }
    else
    {
      out << "    arguments." << p.name << " = " << p.name << "Arg.getValue();\n";
    }
  }
  out << "    arguments.echoSwitch = standardArguments.Echo.getValue();\n"
      << "    arguments.xmlSwitch = standardArguments.XML.getValue();\n"
      << "    arguments.processInformationAddressString = standardArguments.ProcessInformationAddress.getValue();\n"
      << "    arguments.returnParameterFile = standardArguments.ReturnParameterFile.getValue();\n"
      << "  }\n"
      << "  catch (TCLAP::ArgException& e)\n"
      << "  {\n"
      << "    std::cerr << \"error: \" << e.error() << \" for arg \" << e.argId() << std::endl;\n"
      << "    exitStatus = EXIT_FAILURE;\n"
      << "    return false;\n"
      << "  }\n"
      << "\n"
      << "  if (arguments.echoSwitch)\n"
      << "  {\n"
      << "    std::cout << \"Command Line Arguments\" << std::endl;\n";
  for (std::size_t i = 0; i < parameters.size(); ++i)
  {
    out << "    dcmqiCLP::EchoValue(\"" << parameters[i].name << "\", arguments." << parameters[i].name << ");\n";
  }
  out << "    dcmqiCLP::EchoValue(\"echoSwitch\", arguments.echoSwitch);\n"
      << "    dcmqiCLP::EchoValue(\"xmlSwitch\", arguments.xmlSwitch);\n"
      << "    dcmqiCLP::EchoValue(\"processInformationAddressString\", arguments.processInformationAddressString);\n"
      << "    dcmqiCLP::EchoValue(\"returnParameterFile\", arguments.returnParameterFile);\n"
      << "  }\n"
      << "  return true;\n"
      << "}\n"
      << "\n"
      << "} // namespace " << ns << "\n"
      << "\n"
      << "/// Declares the command-line parameters of " << module.name << " as local\n"
      << "/// variables and fills them from argc/argv. Returns from main() after --help,\n"
      << "/// --version and --xml, and on invalid arguments.\n"
      << "#define PARSE_ARGS \\\n"
      << "  " << ns << "::Arguments dcmqiCLPArguments; \\\n"
      << "  { \\\n"
      << "    int dcmqiCLPExitStatus = EXIT_SUCCESS; \\\n"
      << "    if (!" << ns << "::ParseArguments(argc, argv, dcmqiCLPArguments, dcmqiCLPExitStatus)) \\\n"
      << "    { \\\n"
      << "      return dcmqiCLPExitStatus; \\\n"
      << "    } \\\n"
      << "  } \\\n";
  std::vector<std::pair<std::string, std::string> > locals;
  for (std::size_t i = 0; i < parameters.size(); ++i)
  {
    locals.push_back(std::make_pair(CppType(parameters[i].type), parameters[i].name));
  }
  locals.push_back(std::make_pair(std::string("bool"), std::string("echoSwitch")));
  locals.push_back(std::make_pair(std::string("bool"), std::string("xmlSwitch")));
  locals.push_back(std::make_pair(std::string("std::string"), std::string("processInformationAddressString")));
  locals.push_back(std::make_pair(std::string("std::string"), std::string("returnParameterFile")));
  for (std::size_t i = 0; i < locals.size(); ++i)
  {
    out << "  " << locals[i].first << " " << locals[i].second << " = dcmqiCLPArguments." << locals[i].second
        << "; \\\n";
  }
  for (std::size_t i = 0; i < locals.size(); ++i)
  {
    out << "  static_cast<void>(" << locals[i].second << ")" << (i + 1 < locals.size() ? "; \\\n" : "\n");
  }
  out << "\n"
      << "#endif\n";
}

std::string BaseName(const std::string& path)
{
  const std::string::size_type slash = path.find_last_of("/\\");
  return slash == std::string::npos ? path : path.substr(slash + 1);
}

} // namespace

//----------------------------------------------------------------------------
int main(int argc, char* argv[])
{
  if (argc != 3)
  {
    std::cerr << "Usage: " << (argc > 0 ? argv[0] : "dcmqiGenerateCLP") << " <module>.xml <output-header>"
              << std::endl;
    return EXIT_FAILURE;
  }
  const std::string xmlPath = argv[1];
  const std::string headerPath = argv[2];

  std::ifstream xmlStream(xmlPath.c_str(), std::ios::in | std::ios::binary);
  if (!xmlStream)
  {
    std::cerr << xmlPath << ": error: cannot read file" << std::endl;
    return EXIT_FAILURE;
  }
  const std::string xml((std::istreambuf_iterator<char>(xmlStream)), std::istreambuf_iterator<char>());

  Module module;
  try
  {
    module = ReadDescriptor(xml);
    const std::string xmlFileName = BaseName(xmlPath);
    module.name = xmlFileName.substr(0, xmlFileName.rfind('.'));
    if (!IsIdentifier(module.name))
    {
      throw DescriptorError("the file name must be <module>.xml, with <module> a C++ identifier");
    }
  }
  catch (const DescriptorError& e)
  {
    std::cerr << xmlPath << ": error: " << e.what() << std::endl;
    return EXIT_FAILURE;
  }

  std::ostringstream header;
  WriteHeader(header, module, BaseName(xmlPath), xml);

  std::ofstream headerStream(headerPath.c_str(), std::ios::out | std::ios::binary | std::ios::trunc);
  headerStream << header.str();
  headerStream.close();
  if (!headerStream)
  {
    std::cerr << headerPath << ": error: cannot write file" << std::endl;
    return EXIT_FAILURE;
  }
  return EXIT_SUCCESS;
}
