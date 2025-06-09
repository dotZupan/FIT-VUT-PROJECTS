-- Autori: Matus Fignar (xfignam00),
--         Tibor Malega (xmalegt00)
-- 
-- zadanie č. 37 - Evidence řidičů včetně dopravních přestupků, evidence motorových vozidel

-- region DROP TABLE
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
-- end region

-- Vytvaranie sekvencii
CREATE SEQUENCE priestupok_seq START WITH 1 INCREMENT BY 1;
CREATE SEQUENCE policajt_seq START WITH 1 INCREMENT BY 1;


-- region CREATE TABLE
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
-- end region


-- region TRIGGER --
-- Automaticke nastaveniu stavu zaplatenia priestupku na Ano pokial je priestupok s hodnotou 0
CREATE OR REPLACE TRIGGER nastavenie_na_ano
    BEFORE INSERT OR UPDATE ON Priestupok
    FOR EACH ROW
    BEGIN
        IF :NEW.Hodnota = 0 THEN
            :NEW.Zaplatene := 'Ano';
        END IF;
    END;
    /

-- Automatické nastavenie pokutovanej osoby, keď je pokutované vozidlo
CREATE OR REPLACE TRIGGER prirad_pokutovanu_osobu
BEFORE INSERT ON Priestupok
FOR EACH ROW
DECLARE
    v_vlastnik Vozidlo.Vlastnik%TYPE;
BEGIN
    IF :NEW.Pokutovane_vozidlo IS NOT NULL THEN
        SELECT Vlastnik INTO v_vlastnik
        FROM Vozidlo
        WHERE SPZ = :NEW.Pokutovane_vozidlo;

        :NEW.Pokutovana_osoba := v_vlastnik;
    END IF;
EXCEPTION
    WHEN NO_DATA_FOUND THEN
        -- Ak vozidlo nenájdeme, nič nenastavíme
        NULL;
END;
/

-- nastavenie datumu vydania vodičského preukazu podľa aktualneho datumu a datumu platnosti (15 rokov od vydania)
CREATE OR REPLACE TRIGGER nastav_datumy_vodicak
BEFORE INSERT OR UPDATE ON Vodicsky_preukaz
FOR EACH ROW
BEGIN
    IF :NEW.Datum_vydania IS NULL THEN
        :NEW.Datum_vydania := SYSDATE;
    END IF;

    IF :NEW.Datum_platnosti IS NULL THEN
        :NEW.Datum_platnosti := ADD_MONTHS(:NEW.Datum_vydania, 180); -- 15 rokov = 180 mesiacov
    END IF;

    IF :NEW.Datum_platnosti < SYSDATE THEN
        :NEW.Stav := 'Expirovany';
    END IF;
END;
/

-- Automatické nastavenie datumu priestupku, podľa aktualneho datumu
CREATE OR REPLACE TRIGGER nastav_datum_priestupku
BEFORE INSERT OR UPDATE ON Priestupok
FOR EACH ROW
BEGIN
    IF :NEW.Datum IS NULL THEN
        :NEW.Datum := SYSDATE;
    END IF;
END;
/
-- end region --

-- region INSERT --
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
INSERT INTO Osoba (Cislo_OP, Meno, Priezvisko, Rodne_cislo, Datum_narodenia, Adresa, Pohlavie)
VALUES ('XY654321', 'Daniela', 'Mikulova', '7852285432', TO_DATE('1978-02-28', 'YYYY-MM-DD'), 'Lúčna 8, Nitra', 'F');
INSERT INTO Osoba (Cislo_OP, Meno, Priezvisko, Rodne_cislo, Datum_narodenia, Adresa, Pohlavie)
VALUES ('MN908172', 'Nina', 'Sladkovicova', '0651027891', TO_DATE('2006-01-02', 'YYYY-MM-DD'), 'Hviezdoslavova 45, Zvolen', 'F');
INSERT INTO Osoba (Cislo_OP, Meno, Priezvisko, Rodne_cislo, Datum_narodenia, Adresa, Pohlavie)
VALUES ('CD482910', 'Dano', 'Drevo', '9506151234', TO_DATE('1995-06-15', 'YYYY-MM-DD'), 'Jarná 12, Košice', 'M');
INSERT INTO Osoba (Cislo_OP, Meno, Priezvisko, Rodne_cislo, Datum_narodenia, Adresa, Pohlavie)
VALUES ('EF739204', 'Samuel', 'Kupec', '6409115432', TO_DATE('1964-09-11', 'YYYY-MM-DD'), 'Letná 5, Bratislava', 'M');
INSERT INTO Osoba (Cislo_OP, Meno, Priezvisko, Rodne_cislo, Datum_narodenia, Adresa, Pohlavie)
VALUES ('GH120398', 'Stefan', 'Biros', '9404226789', TO_DATE('1994-04-22', 'YYYY-MM-DD'), 'Školská 10, Žilina', 'M');
INSERT INTO Osoba (Cislo_OP, Meno, Priezvisko, Rodne_cislo, Datum_narodenia, Adresa, Pohlavie)
VALUES ('JK582967', 'Timea', 'Kralovicova', '0455174321', TO_DATE('2004-05-17', 'YYYY-MM-DD'), 'SNP 22, Prešov', 'F');



INSERT INTO Policajt (Cislo_policajta, Stredisko)
VALUES ('AB123456', 'ST001');
INSERT INTO Policajt (Cislo_policajta, Stredisko)
VALUES ('CX987654', 'ST002');
INSERT INTO Policajt (Cislo_policajta, Stredisko)
VALUES ('GH120398', 'ST001');


INSERT INTO Vodicsky_preukaz (Cislo_vp, Datum_vydania, Stav, Opravnenie, Vlastnik, Vydavatel)
VALUES ('A1234567', TO_DATE('2022-08-30', 'YYYY-MM-DD'), 'Aktivny', 'B', 'AB123456', 'ST001');
INSERT INTO Vodicsky_preukaz (Cislo_vp, Stav, Opravnenie, Vlastnik, Vydavatel)
VALUES ('R1234567', 'Aktivny', 'B', 'HY123543', 'ST001');
INSERT INTO Vodicsky_preukaz (Cislo_vp, Stav, Opravnenie, Vlastnik, Vydavatel)
VALUES ('B8756238', 'Aktivny', 'B', 'LM564738', 'ST001');
INSERT INTO Vodicsky_preukaz (Cislo_vp, Datum_vydania, Stav, Opravnenie, Vlastnik, Vydavatel)
VALUES ('P0976471', TO_DATE('2022-04-01', 'YYYY-MM-DD'), 'Strateny', 'B', 'HY123543', 'ST001');
INSERT INTO Vodicsky_preukaz (Cislo_vp, Datum_vydania, Stav, Opravnenie, Vlastnik, Vydavatel)
VALUES ('D4560123', TO_DATE('2010-07-13', 'YYYY-MM-DD'), 'Aktivny', 'A,B', 'CX987654', 'ST002');
INSERT INTO Vodicsky_preukaz (Cislo_vp, Datum_vydania, Opravnenie, Vlastnik, Vydavatel)
VALUES ('E0984320', TO_DATE('2010-03-28', 'YYYY-MM-DD'), 'B', 'LM564738', 'ST002');
INSERT INTO Vodicsky_preukaz (Cislo_vp, Datum_vydania, Stav, Opravnenie, Vlastnik, Vydavatel)
VALUES ('X6543217', TO_DATE('2010-09-15', 'YYYY-MM-DD'), 'Aktivny', 'B', 'XY654321', 'ST001');
INSERT INTO Vodicsky_preukaz (Cislo_vp, Datum_vydania, Stav, Opravnenie, Vlastnik, Vydavatel)
VALUES ('M9081723', TO_DATE('2023-06-20', 'YYYY-MM-DD'), 'Aktivny', 'B', 'MN908172', 'ST001');
INSERT INTO Vodicsky_preukaz (Cislo_vp, Datum_vydania, Stav, Opravnenie, Vlastnik, Vydavatel)
VALUES ('C4829109', TO_DATE('2022-11-10', 'YYYY-MM-DD'), 'Aktivny', 'A', 'CD482910', 'ST002');
INSERT INTO Vodicsky_preukaz (Cislo_vp, Datum_vydania, Stav, Opravnenie, Vlastnik, Vydavatel)
VALUES ('L3792401', TO_DATE('2011-07-05', 'YYYY-MM-DD'), 'Aktivny', 'B', 'EF739204', 'ST001');
INSERT INTO Vodicsky_preukaz (Cislo_vp, Datum_vydania, Opravnenie, Vlastnik, Vydavatel)
VALUES ('K2115649', TO_DATE('1996-07-05', 'YYYY-MM-DD'), 'B', 'EF739204', 'ST001');
INSERT INTO Vodicsky_preukaz (Cislo_vp, Datum_vydania, Stav, Opravnenie, Vlastnik, Vydavatel)
VALUES ('N9803985', TO_DATE('2018-03-10', 'YYYY-MM-DD'), 'Aktivny', 'A,B', 'GH120398', 'ST001');
INSERT INTO Vodicsky_preukaz (Cislo_vp, Datum_vydania, Stav, Opravnenie, Vlastnik, Vydavatel)
VALUES ('Z8427903', TO_DATE('2012-03-10', 'YYYY-MM-DD'), 'Strateny', 'A,B', 'GH120398', 'ST001');
INSERT INTO Vodicsky_preukaz (Cislo_vp, Datum_vydania, Stav, Opravnenie, Vlastnik, Vydavatel)
VALUES ('A9609676', TO_DATE('2024-01-25', 'YYYY-MM-DD'), 'Aktivny', 'B', 'JK582967', 'ST002');

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
INSERT INTO Vozidlo (SPZ, VIN, Datum_platnosti_STK, Datum_platnosti_PZP, Znacka, MODEL, Farba, Kategoria, Vlastnik, Evidovalo)
VALUES ('NR123XY', '9BWZZZ377VT004251', TO_DATE('2026-06-10', 'YYYY-MM-DD'), TO_DATE('2027-01-01', 'YYYY-MM-DD'), 'Renault', 'Clio', 'Zelena', 'M', 'XY654321', 'ST001');
INSERT INTO Vozidlo (SPZ, VIN, Datum_platnosti_STK, Datum_platnosti_PZP, Znacka, MODEL, Farba, Kategoria, Vlastnik, Evidovalo)
VALUES ('BA888EF', '1ZVHT82H485113456', TO_DATE('2025-10-12', 'YYYY-MM-DD'), TO_DATE('2026-04-20', 'YYYY-MM-DD'), 'Ford', 'Focus', 'Biela', 'M', 'EF739204', 'ST001');
INSERT INTO Vozidlo (SPZ, VIN, Datum_platnosti_STK, Datum_platnosti_PZP, Znacka, MODEL, Farba, Kategoria, Vlastnik, Evidovalo)
VALUES ('ZV001CD', '1ZVTL82H689117821', TO_DATE('2028-10-18', 'YYYY-MM-DD'), TO_DATE('2027-05-11', 'YYYY-MM-DD'), 'Suzuki', 'Swift', 'červena', 'M', 'MN908172', 'ST002');
INSERT INTO Vozidlo (SPZ, VIN, Datum_platnosti_STK, Datum_platnosti_PZP, Znacka, MODEL, Farba, Kategoria, Vlastnik, Evidovalo)
VALUES ('KE911CD', 'VF3CC8HR8ET000123', TO_DATE('2024-12-01', 'YYYY-MM-DD'), TO_DATE('2025-06-30', 'YYYY-MM-DD'), 'Peugeot', '208', 'Strieborna', 'M', 'CD482910', 'ST002');



INSERT INTO Priestupok (Hodnota, Miesto, Zaplatene, Pokutovana_osoba, Policajt)
VALUES (200, 'Gerlachov', 'Ano', 'HY123543', 'AB123456');
INSERT INTO Priestupok (Hodnota, Datum, Miesto, Pokutovane_vozidlo, Policajt)
VALUES (0, TO_DATE('2025-03-29', 'YYYY-MM-DD'), 'Bratislava', 'BJ324CX', 'AB123456');
INSERT INTO Priestupok (Hodnota, Datum, Miesto, Zaplatene, Pokutovane_vozidlo, Policajt)
VALUES (150, TO_DATE('2024-09-13', 'YYYY-MM-DD'), 'Kosice', 'Nie', 'PO979AC', 'CX987654');
INSERT INTO Priestupok (Hodnota, Miesto, Zaplatene, Pokutovane_vozidlo, Policajt)
VALUES (50, 'Nitra', 'Nie', 'NR123XY', 'GH120398');
INSERT INTO Priestupok (Hodnota, Miesto, Zaplatene, Pokutovana_osoba, Policajt)
VALUES (100, 'Zvolen', 'Ano', 'MN908172', 'CX987654');
INSERT INTO Priestupok (Hodnota, Miesto, Zaplatene, Pokutovane_vozidlo, Policajt)
VALUES (0, 'Bratislava', NULL, 'BA888EF', 'AB123456');
INSERT INTO Priestupok (Hodnota, Miesto, Zaplatene, Pokutovana_osoba, Policajt)
VALUES (75, 'Presov', 'Nie', 'JK582967', 'GH120398');
INSERT INTO Priestupok (Hodnota, Miesto, Zaplatene, Pokutovane_vozidlo, Policajt)
VALUES (120, 'Bratislava', 'Nie', 'BA888EF', 'CX987654');
INSERT INTO Priestupok (Hodnota, Miesto, Zaplatene, Pokutovane_vozidlo, Policajt)
VALUES (800, 'Trnava', 'Ano', 'NR123XY', 'AB123456');
INSERT INTO Priestupok (Hodnota, Miesto, Pokutovana_osoba, Policajt)
VALUES (0, 'Presov', 'XY654321', 'CX987654');
INSERT INTO Priestupok (Hodnota, Miesto, Zaplatene, Pokutovana_osoba, Policajt)
VALUES (300, 'Kosice', 'Nie', 'EF739204', 'GH120398');



INSERT INTO Zaevidovanie_odcudzenia (SPZ, ID_strediska, Datumn_odcudzenia, Datum_najdenia)
VALUES ('BA123AB', 'ST001', TO_DATE('2025-03-01', 'YYYY-MM-DD'), NULL);
INSERT INTO Zaevidovanie_odcudzenia (SPZ, ID_strediska, Datumn_odcudzenia, Datum_najdenia)
VALUES ('PO979AC', 'ST002', TO_DATE('2025-01-21', 'YYYY-MM-DD'), TO_DATE('2025-02-01', 'YYYY-MM-DD'));
INSERT INTO Zaevidovanie_odcudzenia (SPZ, ID_strediska, Datumn_odcudzenia, Datum_najdenia)
VALUES ('KE911CD', 'ST002', TO_DATE('2024-11-15', 'YYYY-MM-DD'), NULL);
-- end region --




-- region SELECT --
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
-- end region --




-- region PROCEDURES
-- procedúra na vypísanie zoznamu a počtu vodičských preukazov, ktoré sú menej ako 6 mesiacov pred expirácou
CREATE OR REPLACE PROCEDURE pred_expiraciou IS
    CURSOR c_preukazy IS
        SELECT Cislo_vp, Datum_platnosti
        FROM Vodicsky_preukaz
        WHERE Datum_platnosti > SYSDATE
        AND Datum_platnosti < SYSDATE + 180;

    v_pocet INTEGER := 0;
BEGIN
    FOR r IN c_preukazy LOOP
        DBMS_OUTPUT.PUT_LINE('Vodičák: ' || r.Cislo_vp || ' platí do: ' || TO_CHAR(r.Datum_platnosti, 'YYYY-MM-DD'));
        v_pocet := v_pocet + 1;
    END LOOP;

    IF v_pocet = 0 THEN
        DBMS_OUTPUT.PUT_LINE('Žiadny vodičák neexpiruje v najbližších 6 mesiacoch.');
    ELSE
        DBMS_OUTPUT.PUT_LINE('Celkovo vodičákov na expiráciu: ' || v_pocet);
    END IF;
END;
/
-- ukážka procedúry
BEGIN
    pred_expiraciou;
END;
/

-- procedúra na nájdenie vlastníka daného vozidla
CREATE OR REPLACE PROCEDURE najdi_vlastnika_spz(p_spz IN Vozidlo.SPZ%TYPE) IS
    v_vlastnik Osoba%ROWTYPE;
BEGIN
    SELECT o.*
    INTO v_vlastnik
    FROM Vozidlo v
    JOIN Osoba o ON v.Vlastnik = o.Cislo_OP
    WHERE v.SPZ = p_spz;

    DBMS_OUTPUT.PUT_LINE('Vlastnik vozidla so SPZ ' || p_spz || ' je: ' || v_vlastnik.Meno || ' ' || v_vlastnik.Priezvisko);
EXCEPTION
    WHEN NO_DATA_FOUND THEN
        DBMS_OUTPUT.PUT_LINE('Vozidlo so SPZ ' || p_spz || ' nebolo najdene.');
END;
/
-- ukážka procedury s platným aj neplatným SPZ
BEGIN
    najdi_vlastnika_spz('BJ324CX');
END;
/
BEGIN
    najdi_vlastnika_spz('KE555TT');
END;
/
BEGIN
    najdi_vlastnika_spz('AA000AA');
END;
/

-- procedura na zistenie či vozidlo je zaevidované ako odcudzené alebo nie
CREATE OR REPLACE PROCEDURE zisti_odcudzenie(p_spz IN Vozidlo.SPZ%TYPE) IS
    v_datum_najdenia Zaevidovanie_odcudzenia.Datum_najdenia%TYPE;
BEGIN
    SELECT Datum_najdenia
    INTO v_datum_najdenia
    FROM Zaevidovanie_odcudzenia
    WHERE SPZ = p_spz;

    IF v_datum_najdenia IS NULL THEN
        DBMS_OUTPUT.PUT_LINE('Vozidlo so SPZ ' || p_spz || ' je aktuálne odcudzené.');
    ELSE
        DBMS_OUTPUT.PUT_LINE('Vozidlo so SPZ ' || p_spz || ' bolo nájdené.');
    END IF;

EXCEPTION
    WHEN NO_DATA_FOUND THEN
        DBMS_OUTPUT.PUT_LINE('Vozidlo so SPZ ' || p_spz || ' nie je evidované ako odcudzené.');
END;
/
-- ukážky procedruy
BEGIN
    zisti_odcudzenie('BA123AB');
END;
/
BEGIN
    zisti_odcudzenie('KE555TT');
END;
/

-- end region

-- region EXPLAIN PLAN --
-- EXPLAIN PLAN pred vytvorením indexu
EXPLAIN PLAN FOR
SELECT o.Pohlavie, COUNT(p.ID_priestupku) AS Pocet_priestupkov
FROM Priestupok p
JOIN Osoba o ON p.Pokutovana_osoba = o.Cislo_OP
GROUP BY o.Pohlavie;

-- Výpis plánu vykonania
SELECT * FROM TABLE(DBMS_XPLAN.DISPLAY);

-- Vytvorenie indexu pre optimalizáciu spojenia
CREATE INDEX idx_priestupok_pokutovana_osoba ON Priestupok(Pokutovana_osoba);
--

-- EXPLAIN PLAN po vytvorení indexu
EXPLAIN PLAN FOR
SELECT o.Pohlavie, COUNT(p.ID_priestupku) AS Pocet_priestupkov
FROM Priestupok p
JOIN Osoba o ON p.Pokutovana_osoba = o.Cislo_OP
GROUP BY o.Pohlavie;

-- Výpis plánu vykonania
SELECT * FROM TABLE(DBMS_XPLAN.DISPLAY);
-- end region --

-- region SELECT WITH CASE
-- vypís pre každú osobu či má nezaplatené priestupky a ak áno tak aj hodnotu nezaplatených priestupkov
WITH Nezaplatenia AS (
    SELECT
        o.Cislo_OP,
        o.Meno,
        o.Priezvisko,
        COUNT(p.ID_priestupku) AS Pocet_nezaplatenych,
        NVL(SUM(p.Hodnota), 0) AS Suma_nezaplatenych  -- Súčet hodnôt nezaplatených priestupkov (ak nemá žiadny, výsledok bude 0 pomocou NVL)
    FROM Osoba o
    LEFT JOIN Priestupok p
        ON (p.Pokutovana_osoba = o.Cislo_OP OR p.Pokutovane_vozidlo IN (
            SELECT v.SPZ
            FROM Vozidlo v
            WHERE v.Vlastnik = o.Cislo_OP
        ))
        AND p.Zaplatene = 'Nie'
    GROUP BY o.Cislo_OP, o.Meno, o.Priezvisko
)
SELECT
    Cislo_OP,
    Meno,
    Priezvisko,
    CASE
        WHEN Pocet_nezaplatenych = 0 THEN 'Žiadny nezaplatený priestupok'
        WHEN Pocet_nezaplatenych = 1 THEN '1 nezaplatený priestupok v hodnote ' || Suma_nezaplatenych
        ELSE TO_CHAR(Pocet_nezaplatenych) || ' nezaplatené priestupky v celkovej hodnote ' || Suma_nezaplatenych
    END AS Stav_priestupkov
FROM Nezaplatenia
ORDER BY Meno, Priezvisko;
-- end region


-- Prístup k tabulkám
GRANT ALL ON Stredisko TO xmalegt00;
GRANT ALL ON Osoba TO xmalegt00;
GRANT ALL ON Policajt TO xmalegt00;
GRANT ALL ON Vozidlo TO xmalegt00;
GRANT ALL ON Vodicsky_preukaz TO xmalegt00;
GRANT ALL ON Priestupok TO xmalegt00;
GRANT ALL ON Zaevidovanie_odcudzenia TO xmalegt00;

-- Prístup na spúšťanie procedúr
GRANT EXECUTE ON pred_expiraciou TO xmalegt00;
GRANT EXECUTE ON najdi_vlastnika_spz TO xmalegt00;
GRANT EXECUTE ON zisti_odcudzenie TO xmalegt00;

COMMIT;