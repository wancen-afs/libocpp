PRAGMA foreign_keys = ON;

-- Authorization cache --
CREATE TABLE IF NOT EXISTS AUTH_CACHE(
    ID_TOKEN_HASH TEXT PRIMARY KEY NOT NULL,
    ID_TOKEN_INFO TEXT NOT NULL,
    LAST_USED INT64 NOT NULL,
    EXPIRY_DATE INT64
);

-- OCPP 2.0.1. Availability --
-- Rows are the operative state (Operative/Inoperative) of the CS, all EVSEs, and all Connectors
CREATE TABLE IF NOT EXISTS AVAILABILITY(
    EVSE_ID INT NOT NULL, -- EVSE_ID=0 AND CONNECTOR_ID=0 addresses the whole CS
    CONNECTOR_ID INT NOT NULL, -- A CONNECTOR_ID of 0 when EVSE_ID > 0 addresses the whole connector
    OPERATIONAL_STATUS TEXT NOT NULL, -- "Operative" or "Inoperative"
    PRIMARY KEY (EVSE_ID, CONNECTOR_ID),
    -- Consistency check: EVSE and connector IDs are positive, and if EVSE_ID is 0, CONNECTOR_ID must also be 0
    CHECK (EVSE_ID >= 0 AND CONNECTOR_ID >= 0 AND (EVSE_ID > 0 OR CONNECTOR_ID = 0))
);

CREATE TABLE IF NOT EXISTS TRANSACTION_QUEUE(
    UNIQUE_ID TEXT PRIMARY KEY NOT NULL,
    MESSAGE TEXT NOT NULL,
    MESSAGE_TYPE TEXT NOT NULL,
    MESSAGE_ATTEMPTS INT NOT NULL,
    MESSAGE_TIMESTAMP TEXT NOT NULL
);


-- Auth list --
CREATE TABLE IF NOT EXISTS AUTH_LIST_VERSION (
    ID INT PRIMARY KEY NOT NULL,
    VERSION INT
);

CREATE TABLE IF NOT EXISTS AUTH_LIST (
    ID_TOKEN_HASH TEXT PRIMARY KEY NOT NULL,
    ID_TOKEN_INFO TEXT NOT NULL
);

INSERT OR IGNORE INTO AUTH_LIST_VERSION (ID, VERSION) VALUES
    (0, 0);


-- Metervalues --
CREATE TABLE IF NOT EXISTS READING_CONTEXT_ENUM (
    ID INT PRIMARY KEY,
    READING_CONTEXT TEXT
);

CREATE TABLE IF NOT EXISTS MEASURAND_ENUM (
    ID INT PRIMARY KEY,
    MEASURAND TEXT
);

CREATE TABLE IF NOT EXISTS PHASE_ENUM (
    ID INT PRIMARY KEY,
    PHASE TEXT
);

CREATE TABLE IF NOT EXISTS LOCATION_ENUM (
    ID INT PRIMARY KEY,
    LOCATION TEXT
);

CREATE TABLE IF NOT EXISTS METER_VALUES (
    ROWID INTEGER PRIMARY KEY,
    TRANSACTION_ID TEXT NOT NULL,
    TIMESTAMP INT64 NOT NULL,
    READING_CONTEXT INTEGER REFERENCES READING_CONTEXT_ENUM (ID),
    CUSTOM_DATA TEXT,
    UNIQUE(TRANSACTION_ID, TIMESTAMP, READING_CONTEXT)
);

CREATE TABLE IF NOT EXISTS METER_VALUE_ITEMS (
    METER_VALUE_ID INTEGER REFERENCES METER_VALUES (ROWID),
    VALUE REAL NOT NULL,
    MEASURAND INTEGER REFERENCES MEASURAND_ENUM (ID),
    PHASE INTEGER REFERENCES PHASE_ENUM (ID),
    LOCATION INTEGER REFERENCES LOCATION_ENUM (ID),
    CUSTOM_DATA TEXT,
    UNIT_CUSTOM_DATA TEXT,
    UNIT_TEXT TEXT,
    UNIT_MULTIPLIER INT
);
