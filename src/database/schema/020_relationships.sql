-- Deduplicate durable keys deterministically: the most recent row wins.
DELETE FROM labels WHERE idLabel NOT IN (SELECT MAX(idLabel) FROM labels GROUP BY fileName);
-- next
DROP INDEX IF EXISTS id_label_filename_idx;
-- next
CREATE UNIQUE INDEX id_label_filename_idx ON labels(fileName);
-- next
ALTER TABLE showsEpisodes RENAME TO oldShowsEpisodes;
-- next
ALTER TABLE showsSettings RENAME TO oldShowsSettings;
-- next
CREATE INDEX migration_settings_dir_idx ON oldShowsSettings(dir,idShow);
-- next
CREATE TEMP TABLE settingsMap AS
SELECT idShow AS oldId, (SELECT MAX(s.idShow) FROM oldShowsSettings s WHERE s.dir=o.dir) AS newId
FROM oldShowsSettings o;
-- next
CREATE TABLE showsSettings (
    "idShow" integer NOT NULL PRIMARY KEY AUTOINCREMENT,
    "tmdbid" text NOT NULL,
    "url" text NOT NULL,
    "showMissingEpisodes" integer NOT NULL,
    "hideSpecialsInMissingEpisodes" integer NOT NULL,
    "dir" text NOT NULL
);
-- next
INSERT INTO showsSettings SELECT * FROM oldShowsSettings WHERE idShow IN (SELECT newId FROM settingsMap);
-- next
CREATE UNIQUE INDEX id_show_settings_dir_idx ON showsSettings(dir);
-- next
CREATE TEMP TABLE mappedEpisodes AS
SELECT e.idEpisode,e.content,m.newId AS idShowSettings,e.seasonNumber,e.episodeNumber,e.tmdbid,e.updated
FROM oldShowsEpisodes e JOIN settingsMap m ON m.oldId=e.idShow;
-- next
CREATE INDEX temp.migration_episode_position_idx ON mappedEpisodes(idShowSettings,seasonNumber,episodeNumber,idEpisode);
-- next
CREATE INDEX temp.migration_episode_tmdb_idx ON mappedEpisodes(idShowSettings,tmdbid,idEpisode);
-- next
CREATE TABLE showsEpisodes (
    "idEpisode" integer NOT NULL PRIMARY KEY AUTOINCREMENT,
    "content" text NOT NULL,
    "idShowSettings" integer NOT NULL REFERENCES showsSettings(idShow) ON DELETE CASCADE,
    "seasonNumber" integer NOT NULL,
    "episodeNumber" integer NOT NULL,
    "tmdbid" text NOT NULL,
    "updated" integer NOT NULL,
    UNIQUE(idShowSettings,seasonNumber,episodeNumber)
);
-- next
INSERT INTO showsEpisodes
SELECT e.* FROM mappedEpisodes e WHERE NOT EXISTS (
    SELECT 1 FROM mappedEpisodes newer WHERE newer.idShowSettings=e.idShowSettings AND newer.idEpisode>e.idEpisode
    AND newer.seasonNumber=e.seasonNumber AND newer.episodeNumber=e.episodeNumber
) AND (e.tmdbid='' OR NOT EXISTS (
    SELECT 1 FROM mappedEpisodes newer WHERE newer.idShowSettings=e.idShowSettings AND newer.idEpisode>e.idEpisode
    AND newer.tmdbid=e.tmdbid
));
-- next
CREATE UNIQUE INDEX id_show_episode_tmdb_idx ON showsEpisodes(idShowSettings,tmdbid) WHERE tmdbid<>'';
-- next
DROP TABLE mappedEpisodes;
-- next
DROP TABLE settingsMap;
-- next
DROP TABLE oldShowsEpisodes;
-- next
DROP TABLE oldShowsSettings;
-- next
ALTER TABLE movieFiles RENAME TO oldMovieFiles;
-- next
CREATE TABLE movieFiles (
    "idFile" integer NOT NULL PRIMARY KEY AUTOINCREMENT,
    "idMovie" integer NOT NULL REFERENCES movies(idMovie) ON DELETE CASCADE,
    "file" text NOT NULL,
    UNIQUE(idMovie,file)
);
-- next
INSERT INTO movieFiles SELECT f.* FROM oldMovieFiles f JOIN movies m ON m.idMovie=f.idMovie
WHERE f.idFile=(SELECT MAX(idFile) FROM oldMovieFiles WHERE idMovie=f.idMovie AND file=f.file) ORDER BY f.idFile;
-- next
DROP TABLE oldMovieFiles;
-- next
ALTER TABLE concertFiles RENAME TO oldConcertFiles;
-- next
CREATE TABLE concertFiles (
    "idFile" integer NOT NULL PRIMARY KEY AUTOINCREMENT,
    "idConcert" integer NOT NULL REFERENCES concerts(idConcert) ON DELETE CASCADE,
    "file" text NOT NULL,
    UNIQUE(idConcert,file)
);
-- next
INSERT INTO concertFiles SELECT f.* FROM oldConcertFiles f JOIN concerts c ON c.idConcert=f.idConcert
WHERE f.idFile=(SELECT MAX(idFile) FROM oldConcertFiles WHERE idConcert=f.idConcert AND file=f.file) ORDER BY f.idFile;
-- next
DROP TABLE oldConcertFiles;
-- next
ALTER TABLE episodes RENAME TO oldEpisodes;
-- next
CREATE TABLE episodes (
    "idEpisode" integer NOT NULL PRIMARY KEY AUTOINCREMENT,
    "content" text NOT NULL,
    "idShow" integer NOT NULL REFERENCES shows(idShow) ON DELETE CASCADE,
    "seasonNumber" integer NOT NULL,
    "episodeNumber" integer NOT NULL,
    "path" text NOT NULL
);
-- next
INSERT INTO episodes SELECT e.* FROM oldEpisodes e JOIN shows s ON s.idShow=e.idShow;
-- next
DROP TABLE oldEpisodes;
-- next
ALTER TABLE episodeFiles RENAME TO oldEpisodeFiles;
-- next
CREATE TABLE episodeFiles (
    "idFile" integer NOT NULL PRIMARY KEY AUTOINCREMENT,
    "idEpisode" integer NOT NULL REFERENCES episodes(idEpisode) ON DELETE CASCADE,
    "file" text NOT NULL,
    UNIQUE(idEpisode,file)
);
-- next
INSERT INTO episodeFiles SELECT f.* FROM oldEpisodeFiles f JOIN episodes e ON e.idEpisode=f.idEpisode
WHERE f.idFile=(SELECT MAX(idFile) FROM oldEpisodeFiles WHERE idEpisode=f.idEpisode AND file=f.file) ORDER BY f.idFile;
-- next
DROP TABLE oldEpisodeFiles;
-- next
ALTER TABLE albums RENAME TO oldAlbums;
-- next
CREATE TABLE albums (
    "idAlbum" integer NOT NULL PRIMARY KEY AUTOINCREMENT,
    "idArtist" integer NOT NULL REFERENCES artists(idArtist) ON DELETE CASCADE,
    "content" text NOT NULL,
    "dir" text NOT NULL,
    "path" text NOT NULL
);
-- next
INSERT INTO albums SELECT a.* FROM oldAlbums a JOIN artists r ON r.idArtist=a.idArtist;
-- next
DROP TABLE oldAlbums;
-- next
ALTER TABLE movieSubtitles RENAME TO oldMovieSubtitles;
-- next
CREATE TABLE movieSubtitles (
    "idSubtitle" integer NOT NULL PRIMARY KEY AUTOINCREMENT,
    "idMovie" integer NOT NULL REFERENCES movies(idMovie) ON DELETE CASCADE,
    "files" text NOT NULL,
    "language" text NOT NULL,
    "forced" integer NOT NULL
);
-- next
INSERT INTO movieSubtitles SELECT s.* FROM oldMovieSubtitles s JOIN movies m ON m.idMovie=s.idMovie;
-- next
DROP TABLE oldMovieSubtitles;
-- next
CREATE INDEX id_subtitle_idx ON movieSubtitles(idMovie);
