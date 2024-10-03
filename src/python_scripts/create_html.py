# Read the HTML file and convert it to a C-style string
def html_to_c_string(html_file, output_c_file):
    with open(html_file, 'r') as f:
        html_content = f.read()
    
    # Escape double quotes and backslashes for C compatibility
    html_content = html_content.replace('\\', '\\\\').replace('"', '\\"')
    
    # Break the HTML content into manageable lines for C
    c_lines = ['"{}\\n"'.format(line) for line in html_content.splitlines()]

    # Join the lines with newlines and proper C syntax
    c_string = '\n'.join(c_lines)

    # Write the result to a C file
    with open(output_c_file, 'w') as f:
        f.write('const char html_content[] = \n')
        f.write(c_string + ';\n')

# Example usage
html_to_c_string(r'src\html_files\index.html', r'src\access_point\htmldata.cpp')
