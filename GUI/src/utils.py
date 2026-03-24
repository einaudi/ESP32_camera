import datetime

def timestamp():
    return datetime.datetime.now().strftime("%H:%M:%S")

def dump_rgb_stream_to_txt(data: bytes, width: int, filepath: str):
    """
    Zapisuje strumień bajtów jako tekst w formacie:
    [RR GG BB] [RR GG BB] ... \n

    :param data: surowe dane (bytes)
    :param width: liczba pikseli w wierszu
    :param filepath: plik wyjściowy
    """
    with open(filepath, "w") as f:
        pixel_count = 0

        # Iterujemy co 3 bajty (RGB)
        for i in range(0, len(data), 3):
            chunk = data[i:i+3]

            # Jeśli mamy pełny piksel
            if len(chunk) == 3:
                r, g, b = chunk
                f.write(f"[0x{r:02X} 0x{g:02X} 0x{b:02X}] ")
                pixel_count += 1
            else:
                # Niedomknięty piksel (np. brakujące bajty)
                f.write("[INCOMPLETE ")
                for byte in chunk:
                    f.write(f"0x{byte:02X} ")
                f.write("] ")

            # Nowa linia po osiągnięciu szerokości
            if pixel_count == width:
                f.write("\n")
                pixel_count = 0

        # Jeśli ostatni wiersz niepełny → zakończ linią
        if pixel_count != 0:
            f.write("\n")