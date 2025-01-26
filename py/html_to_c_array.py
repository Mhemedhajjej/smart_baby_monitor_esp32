import gzip
import sys

def html_to_c_array(html_file, output_file):
    with open(html_file, 'rb') as f:
        html_data = f.read()

    compressed_data = gzip.compress(html_data)

    html_file_name = html_file.replace('web_server/', '')

    with open(output_file, 'w') as f:
        f.write(f'// File: {html_file_name}, Size: {len(compressed_data)}\n')
        f.write(f'#define {html_file_name.split(".")[0]}_len {len(compressed_data)}\n')
        f.write(f'const uint8_t {html_file_name.split(".")[0]}[] = {{\n')

        for i, byte in enumerate(compressed_data):
            if i % 16 == 0:
                f.write('\n ')
            f.write(f'0x{byte:02x}, ')

        f.write('\n};\n')

if __name__ == "__main__":
    if len(sys.argv) != 3:
        print("Usage: python3 html_to_c_array.py <input_html_file> <output_c_file>")
        sys.exit(1)

    html_file = sys.argv[1]
    output_file = sys.argv[2]
    html_to_c_array(html_file, output_file)