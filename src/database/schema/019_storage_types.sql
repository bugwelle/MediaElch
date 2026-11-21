-- Normalize legacy QByteArray bindings before enforcing textual identities.

UPDATE movies SET content=CAST(content AS TEXT), path=CAST(path AS TEXT);
-- next

UPDATE movieFiles SET file=CAST(file AS TEXT);
-- next

UPDATE concerts SET content=CAST(content AS TEXT), path=CAST(path AS TEXT);
-- next

UPDATE concertFiles SET file=CAST(file AS TEXT);
-- next

UPDATE shows SET dir=CAST(dir AS TEXT), content=CAST(content AS TEXT), path=CAST(path AS TEXT);
-- next

UPDATE episodes SET content=CAST(content AS TEXT), path=CAST(path AS TEXT);
-- next

UPDATE episodeFiles SET file=CAST(file AS TEXT);
-- next

UPDATE labels SET fileName=CAST(fileName AS TEXT);
-- next

UPDATE importCache SET filename=CAST(filename AS TEXT), type=CAST(type AS TEXT), path=CAST(path AS TEXT);
-- next

UPDATE settings SET value=CAST(value AS TEXT);
-- next

UPDATE artists SET content=CAST(content AS TEXT), dir=CAST(dir AS TEXT), path=CAST(path AS TEXT);
-- next

UPDATE albums SET content=CAST(content AS TEXT), dir=CAST(dir AS TEXT), path=CAST(path AS TEXT);
-- next

UPDATE movieSubtitles SET files=CAST(files AS TEXT), language=CAST(language AS TEXT);
-- next

UPDATE showsSettings SET tmdbid=CAST(tmdbid AS TEXT), url=CAST(url AS TEXT), dir=CAST(dir AS TEXT);
-- next

UPDATE showsEpisodes SET content=CAST(content AS TEXT), tmdbid=CAST(tmdbid AS TEXT);
