; Autor reseni: matus fignar xfignam00

; Projekt 2 - INP 2024
; Vigenerova sifra na architekture MIPS64

; DATA SEGMENT
                .data
msg:            .asciiz "matusfignar"   ; sem doplnte vase "matusfignar"
cipher:         .space  31              ; misto pro zapis zasifrovaneho textu

; zde si muzete nadefinovat vlastni promenne ci konstanty,
; napr. hodnoty posuvu pro jednotlive znaky sifrovacho klice

key:            .asciiz "fig"           ; Kluc pre sifrovanie
key_value:      .space 31

params_sys5:    .space  8               ; misto pro ulozeni adresy pocatku
                                        ; retezce pro vypis pomoci syscall 5
                                        ; (viz nize "funkce" print_string)


; CODE SEGMENT

;	$t0 -> index spravy	
;	$t1 -> znak spravy					
;	$t2 -> index kluca	
;	$t3 -> znak kluca	
;	$t4 -> flagovaci register pre urcenie odcitavania alebo pripocitavania	
;	
;	$t5 -> flagovaci register



                .text
main:           ADDU            $t0, R0, R0                 
                ADDU            $t2, R0, R0                 

encryptLoop:    ; Nacitanie akutalneho znaku
                LB              $t1, msg($t0)                   
                BEQZ            $t1, encryptEnd                 ; Koniec spravy - nulovy znak

                ; Nacitanie aktualneho kluca
                LB              $t3, key($t2)               
                ADDI            $t3, $t3, -96                   ; prevod znaku kluca na posun (ASCII 'a' = 97)

                ; Urcenie smeru, vpred a vzad
                ANDI            $t4, $t0, 1                     ; 0 pokial parny index
                BEQZ            $t4, forwardShift               ; parny index   = posun vpred (pripocitanie)
                SUB             $t3, R0, $t3                    ; nepárny index = posun vzad  (pripocitanie zapornej hodnoty)

forwardShift:   ; Posun znakov podla kluca
                ADD             $t1, $t1, $t3                   ; Aplikacia posunu podla kluca na znak

                ; Kontrola rozsahu ('a' az 'z')
                SLTI            $t5, $t1, 97                    ; $t5 = 1 pokial < 'a'
                BNE             $t5, R0, underflowCorrection
                SLTI            $t5, $t1, 123                   ; $t5 = 1 pokial <= 'z'
                BEQ             $t5, R0, overflowCorrection
                J               storeChar

underflowCorrection:
                ADDI            $t1, $t1, 26                    ; Cyklicky posun nahor (pod 'a')
                J               storeChar

overflowCorrection:
                ADDI            $t1, $t1, -26                   ; Cyklicky posun nadol (nad 'z')

storeChar:      ; Ulozenie zasifrovaneho znaku do vystupu
                SB              $t1, cipher($t0)

                ; Posun indexov spravy a kluca
                ADDI            $t0, $t0, 1                     ; inkrementacia $t0
                ADDI            $t2, $t2, 1                     ; posunutie indexu kluca o 1
                ADDI            $t5, R0, 3                      ; Nastavenie limitu pre kluc
                BNE             $t2, $t5, encryptLoop           ; Pokial index kluca nie je 3, pokracuj v v sifrovani
                ADDU            $t2, R0, R0                     ; Inak resetuj index registra na 0
                J               encryptLoop

encryptEnd:     ; Ukoncenie vystupu
                SB              R0, cipher($t0)                 ; Nulovy znak na koniec vystupu
                DADDI           R4, R0, cipher                  ; Adresa zasifrovaneho textu
                JAL             print_string                    ; Vypis zasifrovaneho textu

; NASLEDUJICI KOD NEMODIFIKUJTE!

                syscall 0   ; halt

print_string:                                                   ; adresa retezce se ocekava v r4
                SW      R4, params_sys5(R0)
                DADDI   R14, R0, params_sys5                    ; adr pro syscall 5 musi do R14
                syscall 5                                       ; systemova procedura - vypis retezce na terminal
                JR      R31                                     ; return - R31 je urcen na return address
