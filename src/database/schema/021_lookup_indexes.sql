CREATE INDEX id_movie_path_idx ON movies(path);
-- next
CREATE INDEX id_concert_path_idx ON concerts(path);
-- next
CREATE INDEX id_show_path_idx ON shows(path);
-- next
CREATE INDEX id_show_dir_idx ON shows(dir);
-- next
CREATE INDEX id_artist_path_idx ON artists(path);
-- next
CREATE INDEX id_album_artist_idx ON albums(idArtist);
-- next
CREATE INDEX id_album_path_idx ON albums(path);
-- next
CREATE INDEX id_episode_show_idx ON episodes(idShow);
-- next
CREATE INDEX id_episode_path_idx ON episodes(path);
-- next
CREATE INDEX id_show_episode_refresh_idx ON showsEpisodes(idShowSettings,updated);
