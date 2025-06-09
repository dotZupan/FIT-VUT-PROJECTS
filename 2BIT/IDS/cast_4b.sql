-- slúži na demonštráciu využitia udelených práv druhému členovi týmiu

DROP MATERIALIZED VIEW Pocet_Vodicakov_Stredisko;

CREATE MATERIALIZED VIEW Pocet_Vodicakov_Stredisko AS
SELECT 
    v.Vydavatel AS ID_Strediska,
    COUNT(v.Cislo_vp) AS Pocet_Vodicakov
FROM 
    xfignam00.Vodicsky_preukaz v
GROUP BY 
    v.Vydavatel;

SELECT * FROM  Pocet_Vodicakov_Stredisko;