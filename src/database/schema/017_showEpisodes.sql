DROP TABLE IF EXISTS showsEpisodes;
-- next

ALTER TABLE showsSettings RENAME TO oldShowsSettings;
-- next

CREATE TABLE IF NOT EXISTS showsEpisodes (
    "idEpisode" integer NOT NULL PRIMARY KEY AUTOINCREMENT,
    "content" text NOT NULL,
    "idShow" integer NOT NULL,
    "seasonNumber" integer NOT NULL,
    "episodeNumber" integer NOT NULL,
    "tmdbid" text NOT NULL,
    "updated" integer NOT NULL
);
-- next

CREATE TABLE IF NOT EXISTS showsSettings (
    "idShow" integer NOT NULL PRIMARY KEY AUTOINCREMENT,
    "tmdbid" text NOT NULL,
    "url" text NOT NULL,
    "showMissingEpisodes" integer NOT NULL,
    "hideSpecialsInMissingEpisodes" integer NOT NULL,
    "dir" text NOT NULL
);
-- next

INSERT INTO showsSettings(idShow, tmdbid, url, showMissingEpisodes, hideSpecialsInMissingEpisodes, dir)
SELECT idShow, '', url, showMissingEpisodes, hideSpecialsInMissingEpisodes, dir FROM oldShowsSettings;
-- next

DROP TABLE oldShowsSettings;
