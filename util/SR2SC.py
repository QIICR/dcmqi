from pydicom.sr import _snomed_dict
import os
import re
import pydicom.sr._snomed_dict

folder = "E:\\work\\QIICR\\dcmqi"
Out_Folder = "E:\\work\\QIICR\\renamed_dcmqi"


def recursive_file_find(address, regexp):
    filelist = os.listdir(address)
    approvedlist=[]
    for filename in filelist:
        fullpath = os.path.join(address, filename)
        if os.path.isdir(fullpath):
            approvedlist.extend(recursive_file_find(fullpath,  regexp))
        elif re.match(regexp, fullpath) is not None:
            approvedlist.append(fullpath)
    return approvedlist


def GetFileString(filename):
    File_object = open(filename, "r")
    try:
        Content = File_object.read()
    except:
        print("Couldn't read the file")
        Content = ""
    File_object.close()
    return Content


def WriteTextInFile(filename, txt):
    folder = os.path.dirname(filename)
    if not os.path.exists(folder):
        os.makedirs(folder)
    File_object = open(filename, "w")
    File_object.write(txt)
    File_object.close()


def FindRegex(regexp, text, extend=[0, 0], printout=False):
    found_iters = re.finditer(regexp, text)
    founds = list(found_iters)
    ii = []
    for mmatch in founds:
        yy = text[mmatch.start() - extend[0]:mmatch.end() + extend[1]]
        counter = "[%04d ]" % len(ii)
        if (printout):
            print(counter + yy)
        ii.append(yy)
    return ii


def ReplaceQuotedText(find_text, rep_text, text):
    pattern = "(\"\s*" + find_text + "\s*\")|('\s*" + find_text + "\s*')"
    replacement = "\"" + rep_text + "\"";
    new_text = re.sub(pattern, replacement, text)
    View = ShowReplaceMent(pattern, replacement, text)
    return [new_text, View]


def FindAndReplace(find_text, rep_text, text):
    newtext = re.sub(find_text, rep_text, text)
    x = ShowReplaceMent(find_text, rep_text, text)
    return [newtext, x]


def ShowReplaceMent(find_text, rep_text, text):
    output = []
    text_seq = FindRegex("\\n.*(" + find_text + ").*\\n", text, [-1, -1])
    for line_txt in text_seq:
        found_iters = re.finditer(find_text, line_txt)
        founds = list(found_iters)
        if len(founds) > 0:
            mmatch = founds[0]
            yy = line_txt[:mmatch.start()] + \
                 "{ [" + line_txt[mmatch.start():mmatch.end()] + "]-->[" + rep_text + "] }" + \
                 line_txt[mmatch.end():]
            output.append(yy)
    return output


dict = _snomed_dict.mapping["SCT"]
details = []

# recursive_file_find(folder, all_files, "(.*\\.cpp$)|(.*\\.h$)|(.*\\.json$)")
all_files = recursive_file_find(folder, "(?!.*\.git.*)")
for f, jj in zip(all_files, range(1, len(all_files))):
    f_content = (GetFileString(f))
    if len(f_content) == 0:
        continue

    [f_content, x] = ReplaceQuotedText("SCT", "SCT", f_content)
    details = x
    [f_content, x] = FindAndReplace(",\s*SRT\s*,", ",SCT,", f_content)
    details.extend(x)
    [f_content, x] = FindAndReplace("SCT", " SCT ", f_content)
    details.extend(x)
    [f_content, x] = FindAndReplace("_SCT_", "_SCT_", f_content)
    details.extend(x)
    [f_content, x] = FindAndReplace("sct.h", "sct.h", f_content)
    details.extend(x)

    for srt_code, sct_code in dict.items():
        # f_content = ReplaceQuotedText(srt_code, sct_code, f_content)
        [f_content, x] = FindAndReplace(srt_code, sct_code, f_content)
        details.extend(x)
    if len(details) == 0:
        continue
    edited_file_name = f.replace(folder, Out_Folder)
    edited_file_log = f.replace(folder, os.path.join(Out_Folder, '..\\log')) + ".txt"
    WriteTextInFile(edited_file_name, f_content)
    print("------------------------------------------------------------------------")
    f_number = "(file %03d ) " % jj
    print(f_number + f)
    logg = ""
    for m, c in zip(details, range(0, len(details))):
        indent = "\t\t\t%04d" % c
        logg += (indent + m + "\n")
    if len(logg) != 0:
        WriteTextInFile(edited_file_log, logg)

print("the find/replace process finished ...")
