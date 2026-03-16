import re
import sys


def clean_comments(input_file, output_file):
    with open(input_file, "r", encoding="utf-8") as f:
        text = f.read()

    # Pattern giữ lại comment dạng * Gxx-Level | BugType
    keep_pattern = re.compile(r"\*\s*G\d{2}-\w+\s*\|\s*[^\n]+")

    kept_comments = {}

    # Tạm thời thay thế comment cần giữ bằng placeholder
    def keep_replacer(match):
        key = f"__KEEP_{len(kept_comments)}__"
        kept_comments[key] = match.group(0)
        return key

    text = keep_pattern.sub(keep_replacer, text)

    # Xóa comment block
    text = re.sub(r"/\*.*?\*/", "", text, flags=re.S)

    # Xóa comment line
    text = re.sub(r"//.*", "", text)

    # Khôi phục comment cần giữ
    for key, value in kept_comments.items():
        text = text.replace(key, value)

    # Remove dòng trống dư
    text = re.sub(r"\n\s*\n", "\n\n", text)

    with open(output_file, "w", encoding="utf-8") as f:
        f.write(text)


if __name__ == "__main__":

    if len(sys.argv) != 3:
        print("Usage:")
        print("python clean_test_comment.py input.cpp output.cpp")
        sys.exit(1)

    clean_comments(sys.argv[1], sys.argv[2])