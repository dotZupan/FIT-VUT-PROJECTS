-- Autori: Matus Fignar (xfignam00), Tibor Malega (xmaleg00)

-- Drop pre vsetky tabulky a sekvencie
DROP TABLE Zaevidovanie_odcudzenia CASCADE CONSTRAINTS;
DROP TABLE Priestupok CASCADE CONSTRAINTS;
DROP TABLE Vodicsky_preukaz CASCADE CONSTRAINTS;
DROP TABLE Vozidlo CASCADE CONSTRAINTS;
DROP TABLE Policajt CASCADE CONSTRAINTS;
DROP TABLE Stredisko CASCADE CONSTRAINTS;
DROP TABLE Osoba CASCADE CONSTRAINTS;
DROP SEQUENCE priestupok_seq;
DROP SEQUENCE policajt_seq;

-- Vytvaranie sekvencii
CREATE SEQUENCE priestupok_seq START WITH 1 INCREMENT BY 1;
CREATE SEQUENCE policajt_seq START WITH 1 INCREMENT BY 1;

-- Vytvaranie tabuliek
CREATE TABLE Stredisko ( 
    ID_strediska         VARCHAR(255) NOT NULL,
    PSC                 VARCHAR(5) NOT NULL,
    Adresa              VARCHAR(255) NOT NULL,
    PRIMARY KEY (ID_strediska),
    CONSTRAINT PSC CHECK(REGEXP_LIKE(PSC, '^[0-9]{5}$'))
);

-- Pretoze Entita Vodica je generalizovana z entitiy Osoba 
-- a nema ziadne atributy ani vztahy s inymi entitami,
-- nemusime vytvarat tabulku pre nu.
CREATE TABLE Osoba (
    Cislo_OP            VARCHAR(8) NOT NULL,
    Meno                VARCHAR(255) NOT NULL,
    Priezvisko          VARCHAR(255) NOT NULL,
    Rodne_cislo         VARCHAR(10) NOT NULL,
    Datum_narodenia     DATE NOT NULL,
    Adresa              VARCHAR(255) NOT NULL,
    Pohlavie            CHAR(1) NOT NULL,
    PRIMARY KEY (Cislo_OP),
    CONSTRAINT Cislo_OP CHECK(REGEXP_LIKE(Cislo_OP, '^[A-Z]{2}[0-9]{6}$')), -- Cislo_vp musi obsahovat 8 znakov, 2 pismena nasledovane 6 cislicami
    CONSTRAINT Rodne_cislo CHECK(REGEXP_LIKE(Rodne_cislo, '^[0-9]{2}(0[1-9]|1[0-2]|5[1-9]|6[0-2])[0-3][0-9][0-9]{3,4}$')), -- Rodne_cislo obshauje 10 znakov, zacina 2mi poslednymi cislicami roka, nasledovane mesiacom (01-12) alebo (51-62), dnom (01-31) a potom 3 alebo 4mi cislami
    CONSTRAINT Pohlavie CHECK(Pohlavie IN ('M', 'F'))
);

CREATE TABLE Policajt (
    Cislo_policajta     VARCHAR(8) NOT NULL,
    Sluzobne_cislo      INT DEFAULT policajt_seq.NEXTVAL NOT NULL,
    Stredisko           VARCHAR(255) NOT NULL,
    FOREIGN KEY (Stredisko) REFERENCES Stredisko (ID_strediska),
    PRIMARY KEY (Cislo_policajta),
    FOREIGN KEY (Cislo_policajta) REFERENCES Osoba (Cislo_OP) ON DELETE CASCADE
);

CREATE TABLE Vozidlo (
    SPZ                 VARCHAR(7) NOT NULL,
    VIN                 VARCHAR(17) NOT NULL,
    Datum_platnosti_STK DATE,
    Datum_platnosti_PZP DATE,
    Znacka              VARCHAR(255) NOT NULL,
    MODEL               VARCHAR(255) NOT NULL,
    Farba               VARCHAR(255) NOT NULL,
    Kategoria           VARCHAR(255) NOT NULL,
    Vlastnik            VARCHAR(8) NOT NULL,
    Evidovalo           VARCHAR(255) NOT NULL,
    PRIMARY KEY (SPZ),
    FOREIGN KEY (Vlastnik) REFERENCES Osoba (Cislo_OP),
    FOREIGN KEY (Evidovalo) REFERENCES Stredisko (ID_strediska),
    CONSTRAINT SPZ CHECK(REGEXP_LIKE(SPZ, '^[A-Z]{2}[0-9]{3}[A-Z]{2}$')), -- SPZ musi byt 7 znakov dlhe, zacina 2 pismenami, nasledovanymi 3 cislicami a zakoncene 2 pismenami, Slovensky format
    CONSTRAINT VIN CHECK(REGEXP_LIKE(VIN, '^[A-HJ-NPR-Z0-9]{17}$'))  -- VIN musi obsahovat 17 znakov a nemoze obsahovat pismena I, O, Q
);

CREATE TABLE Vodicsky_preukaz (
    Cislo_vp            VARCHAR(8) NOT NULL,
    Datum_vydania       DATE NOT NULL,
    Datum_platnosti     DATE NOT NULL,
    Stav                VARCHAR(10) NOT NULL,
    Opravnenie          VARCHAR(255) NOT NULL,
    Vlastnik            VARCHAR(8) NOT NULL,
    Vydavatel           VARCHAR(255) NOT NULL,
    PRIMARY KEY (Cislo_vp),
    FOREIGN KEY (Vlastnik) REFERENCES Osoba (Cislo_OP),
    FOREIGN KEY (Vydavatel) REFERENCES Stredisko (ID_strediska),
    CONSTRAINT Stav CHECK (Stav IN ('Aktivny', 'Odcudzeny', 'Strateny', 'Neplatny', 'Expirovany')), -- Stav moze byt len jeden z tychto 5 stavov
    CONSTRAINT Cislo_vp CHECK(REGEXP_LIKE(Cislo_vp, '^[A-Z][0-9]{7}$')) -- Cislo_vp musi obsahovat 8 znakov, 1 pismeno nasledovane 7 cislicami
);

CREATE TABLE Priestupok (
    ID_priestupku       INT DEFAULT priestupok_seq.NEXTVAL NOT NULL,
    Hodnota             INT,
    Datum               DATE NOT NULL,
    Miesto              VARCHAR(255) NOT NULL,
    Zaplatene           VARCHAR(3),
    Pokutovana_osoba    VARCHAR(8), 
    Pokutovane_vozidlo  VARCHAR(7),
    Policajt            VARCHAR(8) NOT NULL,
    PRIMARY KEY (ID_priestupku),
    FOREIGN KEY (Pokutovane_vozidlo) REFERENCES Vozidlo (SPZ),
    FOREIGN KEY (Pokutovana_osoba) REFERENCES Osoba (Cislo_OP),
    FOREIGN KEY (Policajt) REFERENCES Policajt (Cislo_policajta),
    CONSTRAINT Zaplatene CHECK(Zaplatene IN ('Ano', 'Nie'))
);

CREATE TABLE Zaevidovanie_odcudzenia(
    SPZ                 VARCHAR(7) NOT NULL,
    ID_strediska        VARCHAR(255) NOT NULL,
    Datumn_odcudzenia   DATE NOT NULL,
    Datum_najdenia      DATE,
    PRIMARY KEY (SPZ, ID_strediska),
    FOREIGN KEY (SPZ) REFERENCES Vozidlo (SPZ),
    FOREIGN KEY (ID_strediska) REFERENCES Stredisko (ID_strediska)
);

-- Vkladanie testovacich dat
INSERT INTO Stredisko (ID_strediska, PSC, Adresa)
VALUES ('ST001', '01001', 'Police Station 1, Bratislava');
INSERT INTO Stredisko (ID_strediska, PSC, Adresa)
VALUES ('ST002', '01021', 'Police Station 2, Kosice');

INSERT INTO Osoba (Cislo_OP, Meno, Priezvisko, Rodne_cislo, Datum_narodenia, Adresa, Pohlavie)
VALUES ('AB123456', 'Jozef', 'Novak', '9001011234', TO_DATE('1990-01-01', 'YYYY-MM-DD'), 'Hlavna 123, Bratislava', 'M');
INSERT INTO Osoba (Cislo_OP, Meno, Priezvisko, Rodne_cislo, Datum_narodenia, Adresa, Pohlavie)
VALUES ('HY123543', 'Marek', 'Boruv', '0402204433', TO_DATE('2004-02-20', 'YYYY-MM-DD'), 'Partizanska 13, Bardejov', 'M');
INSERT INTO Osoba (Cislo_OP, Meno, Priezvisko, Rodne_cislo, Datum_narodenia, Adresa, Pohlavie)
VALUES ('CX987654', 'Petra', 'Kovacova', '9553157890', TO_DATE('1995-03-15', 'YYYY-MM-DD'), 'Druzstevna 45, Kosice', 'F');
INSERT INTO Osoba (Cislo_OP, Meno, Priezvisko, Rodne_cislo, Datum_narodenia, Adresa, Pohlavie)
VALUES ('LM564738', 'Ivana', 'Tomasova', '8857203456', TO_DATE('1988-07-20', 'YYYY-MM-DD'), 'Slnečna 7, Presov', 'F');

INSERT INTO Policajt (Cislo_policajta, Stredisko)
VALUES ('AB123456', 'ST001');
INSERT INTO Policajt (Cislo_policajta, Stredisko)
VALUES ('CX987654', 'ST002');

INSERT INTO Vodicsky_preukaz (Cislo_vp, Datum_vydania, Datum_platnosti, Stav, Opravnenie, Vlastnik, Vydavatel)
VALUES ('A1234567', TO_DATE('2024-01-01', 'YYYY-MM-DD'), TO_DATE('2034-01-01', 'YYYY-MM-DD'), 'Aktivny', 'B', 'AB123456', 'ST001');
INSERT INTO Vodicsky_preukaz (Cislo_vp, Datum_vydania, Datum_platnosti, Stav, Opravnenie, Vlastnik, Vydavatel)
VALUES ('R1234567', TO_DATE('2022-04-01', 'YYYY-MM-DD'), TO_DATE('2032-04-01', 'YYYY-MM-DD'), 'Aktivny', 'B', 'HY123543', 'ST001');
INSERT INTO Vodicsky_preukaz (Cislo_vp, Datum_vydania, Datum_platnosti, Stav, Opravnenie, Vlastnik, Vydavatel)
VALUES ('P0976471', TO_DATE('2020-02-01', 'YYYY-MM-DD'), TO_DATE('2030-02-01', 'YYYY-MM-DD'), 'Strateny', 'B', 'HY123543', 'ST001');
INSERT INTO Vodicsky_preukaz (Cislo_vp, Datum_vydania, Datum_platnosti, Stav, Opravnenie, Vlastnik, Vydavatel)
VALUES ('D4560123', TO_DATE('2016-03-02', 'YYYY-MM-DD'), TO_DATE('2026-03-02', 'YYYY-MM-DD'), 'Aktivny', 'A,B', 'CX987654', 'ST002');
INSERT INTO Vodicsky_preukaz (Cislo_vp, Datum_vydania, Datum_platnosti, Stav, Opravnenie, Vlastnik, Vydavatel)
VALUES ('E0984320', TO_DATE('2015-03-28', 'YYYY-MM-DD'), TO_DATE('2025-03-28', 'YYYY-MM-DD'), 'Expirovany', 'B', 'LM564738', 'ST002');

INSERT INTO Vozidlo (SPZ, VIN, Datum_platnosti_STK, Datum_platnosti_PZP, Znacka, MODEL, Farba, Kategoria, Vlastnik, Evidovalo)
VALUES ('BA123AB', '1HGCM82633A123456', TO_DATE('2025-05-15', 'YYYY-MM-DD'), TO_DATE('2025-12-31', 'YYYY-MM-DD'), 'Honda', 'Civic', 'Cervena', 'M', 'HY123543', 'ST001');
INSERT INTO Vozidlo (SPZ, VIN, Datum_platnosti_STK, Datum_platnosti_PZP, Znacka, MODEL, Farba, Kategoria, Vlastnik, Evidovalo)
VALUES ('BJ324CX', '1HGCM82633A678901', TO_DATE('2025-07-15', 'YYYY-MM-DD'), TO_DATE('2026-12-31', 'YYYY-MM-DD'), 'Citroen', 'C5AirCross', 'Seda', 'M', 'HY123543', 'ST001');
INSERT INTO Vozidlo (SPZ, VIN, Datum_platnosti_STK, Datum_platnosti_PZP, Znacka, MODEL, Farba, Kategoria, Vlastnik, Evidovalo)
VALUES ('KE255ZS', 'WVWZZZ1JZXW000123', TO_DATE('2027-04-11', 'YYYY-MM-DD'), TO_DATE('2027-04-25', 'YYYY-MM-DD'), 'Volkswagen', 'Passat', 'Cierna', 'M', 'CX987654', 'ST002');
INSERT INTO Vozidlo (SPZ, VIN, Datum_platnosti_STK, Datum_platnosti_PZP, Znacka, MODEL, Farba, Kategoria, Vlastnik, Evidovalo)
VALUES ('KE555TT', '6ACCT65165C098664', TO_DATE('2029-11-01', 'YYYY-MM-DD'), TO_DATE('2028-02-28', 'YYYY-MM-DD'), 'Honda', 'CBF-600', 'Modra', 'L', 'CX987654', 'ST002');
INSERT INTO Vozidlo (SPZ, VIN, Datum_platnosti_STK, Datum_platnosti_PZP, Znacka, MODEL, Farba, Kategoria, Vlastnik, Evidovalo)
VALUES ('PO979AC', '1HGCM82633A004352', TO_DATE('2026-05-05', 'YYYY-MM-DD'), TO_DATE('2026-10-01', 'YYYY-MM-DD'), 'Skoda', 'Octavia', 'Cierna', 'M', 'LM564738', 'ST002');

INSERT INTO Priestupok (Hodnota, Datum, Miesto, Zaplatene, Pokutovana_osoba, Policajt)
VALUES (200, TO_DATE('2025-03-29', 'YYYY-MM-DD'), 'Gerlachov', 'Ano', 'HY123543', 'AB123456');
INSERT INTO Priestupok (Hodnota, Datum, Miesto, Zaplatene, Pokutovane_vozidlo, Policajt)
VALUES (60, TO_DATE('2025-03-29', 'YYYY-MM-DD'), 'Bratislava', 'Nie', 'BJ324CX', 'AB123456');
INSERT INTO Priestupok (Hodnota, Datum, Miesto, Zaplatene, Pokutovane_vozidlo, Policajt)
VALUES (150, TO_DATE('2024-09-13', 'YYYY-MM-DD'), 'Kosice', 'Nie', 'PO979AC', 'CX987654');

INSERT INTO Zaevidovanie_odcudzenia (SPZ, ID_strediska, Datumn_odcudzenia, Datum_najdenia)
VALUES ('BA123AB', 'ST001', TO_DATE('2025-03-01', 'YYYY-MM-DD'), NULL);
INSERT INTO Zaevidovanie_odcudzenia (SPZ, ID_strediska, Datumn_odcudzenia, Datum_najdenia)
VALUES ('PO979AC', 'ST002', TO_DATE('2025-01-21', 'YYYY-MM-DD'), TO_DATE('2025-02-01', 'YYYY-MM-DD'));

-- Vyhladanie majitela pokutovaneho vozidla
SELECT p.ID_PRIESTUPKU, v.SPZ, o.Cislo_OP, o.Meno, o.Priezvisko
FROM Priestupok p
JOIN Vozidlo v ON p.Pokutovane_vozidlo = v.SPZ
JOIN Osoba o ON o.Cislo_OP = v.Vlastnik;

-- Vypise osoby a ich platne vodicske preukazy
SELECT vp.Cislo_vp, o.Cislo_OP, o.Meno, o.Priezvisko
FROM Vodicsky_preukaz vp JOIN Osoba o ON vp.Vlastnik = o.Cislo_OP
WHERE vp.stav = 'Aktivny';

-- Vypise ake stredisko zaevidovalo vozidlo
SELECT v.SPZ, s.ID_strediska
FROM Vozidlo v JOIN Stredisko s ON v.Evidovalo = s.ID_strediska;

-- vypise pocet vydanych priestupkov policajtov
SELECT p.SLUZOBNE_CISLO, o.meno, o.priezvisko, COUNT(pr.ID_priestupku) AS pocet_priestupkov
FROM Policajt p JOIN Osoba o ON p.Cislo_policajta = o.Cislo_OP
LEFT JOIN Priestupok pr ON p.Cislo_policajta = pr.POLICAJT
GROUP BY p.SLUZOBNE_CISLO, o.meno, o.priezvisko;

-- Vypise kolko policajtov sa nacháva v jednotlivych strediskach
SELECT s.ID_strediska, COUNT(p.Cislo_policajta) AS Pocet_policajtov
FROM Stredisko s
LEFT JOIN Policajt p ON s.ID_strediska = p.Stredisko
GROUP BY s.ID_strediska
ORDER BY s.ID_strediska;

-- Kolko priestupkov napachali osoby
SELECT 
    o.Cislo_OP,
    o.Meno,
    o.Priezvisko,
    COUNT(DISTINCT p.ID_priestupku) AS Pocet_priestupkov
FROM Osoba o
LEFT JOIN Priestupok p
    ON p.Pokutovana_osoba = o.Cislo_OP
    OR p.Pokutovane_vozidlo IN (
        SELECT v.SPZ
        FROM Vozidlo v
        WHERE v.Vlastnik = o.Cislo_OP
    )
GROUP BY o.Cislo_OP, o.Meno, o.Priezvisko;

-- Vypise vozidla, ktoré boli niekedy odcudzené
SELECT v.SPZ, v.Znacka, v.Model, v.Farba
FROM Vozidlo v
WHERE EXISTS (
    SELECT *
    FROM Zaevidovanie_odcudzenia zo
    WHERE zo.SPZ = v.SPZ
);

COMMIT;