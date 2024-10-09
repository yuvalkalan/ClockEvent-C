import time

import pygame
import numpy as np

WIDTH = 10
HEIGHT = 22
FONT_PATH = r'src\python\JetBrainsMono-ExtraLight.ttf'
DST_PATH = r"src\graphics\ST7735\font.h"
# Define colors
WHITE = (255, 255, 255)
BLACK = (0, 0, 0)


def bool_list_to_2byte_number(bool_list):
    num = 0
    for i, value in enumerate(bool_list):
        if value:
            num |= (1 << i)  # Set the bit at position i if the value is True
    return num


def rotate_90_degrees(matrix):
    # Step 1: Transpose the matrix
    transposed_matrix = [list(row) for row in zip(*matrix)]

    # Step 2: Reverse each row
    rotated_matrix = [row[::-1] for row in transposed_matrix]

    return rotated_matrix


def check_font_size():
    current_size = 1
    alive = True
    while alive:
        current_size += 1
        font = pygame.font.Font(FONT_PATH, current_size)
        for i in range(32, 127):
            text_surface = font.render(chr(i), True, WHITE, BLACK)
            r = text_surface.get_rect()
            alive = alive and (r.width <= WIDTH and r.height <= HEIGHT)
    current_size -= 1
    print(f"font size = {current_size}")
    return current_size


def check_biggest_char(font):
    max_w, max_h = 0, 0
    for i in range(32, 127):
        text_surface = font.render(chr(i), True, WHITE, BLACK)
        r = text_surface.get_rect()
        max_w = max(max_w, r.width)
        max_h = max(max_h, r.height)
    return max_w, max_h


def main():
    # Initialize Pygame
    pygame.init()
    screen = pygame.display.set_mode((WIDTH, HEIGHT))
    font = pygame.font.Font(FONT_PATH, check_font_size())
    max_width, max_height = check_biggest_char(font)
    print(max_width, max_height)
    bitmap = []
    for i in range(32, 127):
        text_surface = font.render(chr(i), True, WHITE, BLACK)
        screen.fill(BLACK)
        r = text_surface.get_rect()
        r.center = (max_width / 2, max_height / 2)
        # r.center = (WIDTH//2, HEIGHT//2)
        screen.blit(text_surface, r.topleft)
        # screen.blit(text_surface, (0, 0))
        arr = pygame.surfarray.array2d(screen)
        binary_array = np.where(arr == screen.map_rgb(BLACK), 0, 1)
        real_values = [bool_list_to_2byte_number(lst) for lst in rotate_90_degrees(binary_array.tolist()[::-1])] # mirro and rotate to fit code
        bitmap.append(["0x"+hex(v).replace("0x", '').zfill(4) for v in real_values])
    with open(DST_PATH, "w+") as font_file:
        font_file.write(f"const int FONT_CHAR_HEIGHT = {HEIGHT};\n")
        font_file.write(f"const int FONT_CHAR_WIDTH = {WIDTH};\n")
        font_file.write("const uint16_t font_bitmap[][FONT_CHAR_HEIGHT] = {\n")
        for i in range(len(bitmap)):
            array_string = ', '.join(bitmap[i])
            font_file.write('\t{'+array_string+'}, // \'' + chr(i+32) + '\'\n')
        font_file.write('};\n')


# def test_font():
#     pygame.init()
#     screen = pygame.display.set_mode((WIDTH*100, HEIGHT*10))
#     font = pygame.font.Font(FONT_PATH, check_font_size()*10)
#     while True:
#         pygame.event.get()
#         text_surface = font.render("1234567890", True, WHITE, BLACK)
#         screen.blit(text_surface, (0,0) )
#         pygame.display.flip()


if __name__ == '__main__':
    # test_font()
    main()
