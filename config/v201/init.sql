CREATE TABLE IF NOT EXISTS AUTH_CACHE(
    ID_TOKEN_HASH TEXT PRIMARY KEY NOT NULL,
    ID_TOKEN_INFO TEXT NOT NULL
);