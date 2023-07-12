/* Define helper symbols to inform Cooja about start addresses and sizes
   of segments. */
SECTIONS{
    cooja_bssSize = SIZEOF(.bss);
    cooja_dataSize = SIZEOF(.data);
    cooja_bssStart = ADDR(.bss);
    cooja_dataStart = ADDR(.data);
}
INSERT BEFORE .lbss;
