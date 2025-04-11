DROP TABLE IF EXISTS ShareStockList CASCADE;
DROP TABLE IF EXISTS Review CASCADE;
DROP TABLE IF EXISTS Friends CASCADE;
DROP TABLE IF EXISTS PortfolioHasStock CASCADE;
DROP TABLE IF EXISTS StockListHasStock CASCADE;
DROP TABLE IF EXISTS PredictStockPrice CASCADE;
DROP TABLE IF EXISTS Portfolio CASCADE;
DROP TABLE IF EXISTS StockList CASCADE;
DROP TABLE IF EXISTS Stock CASCADE;
DROP TABLE IF EXISTS "User" CASCADE;

CREATE TABLE "User" (
    username VARCHAR(50) PRIMARY KEY,
    password TEXT NOT NULL
);

CREATE TABLE Portfolio (
    name VARCHAR(50) NOT NULL,
    cashAccount NUMERIC(15, 2) DEFAULT 0,
    ownerUsername VARCHAR(50) NOT NULL,
    PRIMARY KEY (name, ownerUsername),
    FOREIGN KEY (ownerUsername) REFERENCES "User"(username) ON DELETE CASCADE
);

CREATE TABLE StockList (
    visibility VARCHAR(10) CHECK (visibility IN ('public', 'shared', 'private')) DEFAULT 'private',
    name VARCHAR(100) NOT NULL,
    ownerUsername VARCHAR(50) NOT NULL,
    PRIMARY KEY (name, ownerUsername),
    FOREIGN KEY (ownerUsername) REFERENCES "User"(username) ON DELETE CASCADE
);

CREATE TABLE Stock (
    symbol VARCHAR(10) PRIMARY KEY,
);

CREATE TABLE StockHistory (
    symbol VARCHAR(10),
    timestamp DATE,
    open NUMERIC(10, 2),
    close NUMERIC(10, 2),
    high NUMERIC(10, 2),
    low NUMERIC(10, 2),
    volume BIGINT,
    PRIMARY KEY (symbol, timestamp)
);

COPY StockHistory(
    timestamp, 
    open, 
    high,
    low, 
    close, 
    volume, 
    symbol) FROM 'C:\Users\yoson\Downloads\SP500History.csv' DELIMITER ','
CSV HEADER;


CREATE TABLE Review (
    writerUsername VARCHAR(50),
    ownerUsername VARCHAR(50), 
    stockListName VARCHAR(100) NOT NULL,
    content TEXT CHECK (char_length(content) <= 4000),
    createdAt TIMESTAMP DEFAULT CURRENT_TIMESTAMP,
    updatedAt TIMESTAMP DEFAULT CURRENT_TIMESTAMP,
    PRIMARY KEY (writerUsername, ownerUsername, stockListName),
    FOREIGN KEY (writerUsername) REFERENCES "User"(username) ON DELETE CASCADE,
    FOREIGN KEY (stockListName, ownerUsername) REFERENCES StockList(name, ownerUsername) ON DELETE CASCADE
);

CREATE TABLE Friends (
    senderUsername VARCHAR(50),
    receiverUsername VARCHAR(50),
    state VARCHAR(10) DEFAULT 'pending' CHECK (state IN ('pending', 'accepted', 'rejected', 'deleted')),
    requestTime TIMESTAMP DEFAULT CURRENT_TIMESTAMP,
    updatedTime TIMESTAMP DEFAULT CURRENT_TIMESTAMP,
    PRIMARY KEY (senderUsername, receiverUsername),
    FOREIGN KEY (senderUsername) REFERENCES "User"(username) ON DELETE CASCADE,
    FOREIGN KEY (receiverUsername) REFERENCES "User"(username) ON DELETE CASCADE
);

CREATE TABLE ShareStockList (
    ownerUsername VARCHAR(50),
    receiverUsername VARCHAR(50),
    stockListName VARCHAR(100) NOT NULL,
    PRIMARY KEY (ownerUsername, receiverUsername, stockListName),
    FOREIGN KEY (stockListName, ownerUsername) REFERENCES StockList(name, ownerUsername) ON DELETE CASCADE,
    FOREIGN KEY (receiverUsername) REFERENCES "User"(username) ON DELETE CASCADE
);

CREATE TABLE StockListHasStock (
    ownerUsername VARCHAR(50),
    stockListName VARCHAR(100) NOT NULL,
    stockID VARCHAR(10),
    quantity INTEGER NOT NULL,
    PRIMARY KEY (stockListName, ownerUsername, stockID),
    FOREIGN KEY (stockListName, ownerUsername) REFERENCES StockList(name, ownerUsername) ON DELETE CASCADE,
    FOREIGN KEY (stockID) REFERENCES Stock(symbol) ON DELETE CASCADE
);

CREATE TABLE PortfolioHasStock (
    portfolioName VARCHAR(50) NOT NULL,
    ownerUsername VARCHAR(50) NOT NULL,
    stockID VARCHAR(10),
    purchaseDate DATE,
    purchasePrice NUMERIC(10, 2),
    quantity INTEGER,
    sold BOOLEAN DEFAULT FALSE,
    sellDate DATE,
    sellPrice NUMERIC(10, 2),
    PRIMARY KEY (portfolioName, ownerUsername, stockID, purchaseDate),
    FOREIGN KEY (portfolioName, ownerUsername) REFERENCES Portfolio(name, ownerUsername) ON DELETE CASCADE,
    FOREIGN KEY (stockID) REFERENCES Stock(symbol) ON DELETE CASCADE
);

CREATE TABLE PredictStockPrice (
    symbol VARCHAR(10),
    predictedDate DATE,
    predictedPrice NUMERIC(10, 2),
    PRIMARY KEY (symbol, predictedDate),
    FOREIGN KEY (symbol) REFERENCES Stock(symbol) ON DELETE CASCADE
)

CREATE OR REPLACE FUNCTION check_friendship()
RETURNS TRIGGER AS $$
BEGIN
    IF NOT EXISTS (
        SELECT 1 FROM Friends
        WHERE
            accepted = TRUE AND (
                (senderUsername = NEW.ownerUsername AND receiverUsername = NEW.receiverUsername) OR
                (senderUsername = NEW.receiverUsername AND receiverUsername = NEW.ownerUsername)
            )
    ) THEN
        RAISE EXCEPTION 'Users % and % are not friends.', NEW.ownerUsername, NEW.receiverUsername;
    END IF;

    RETURN NEW;
END;
$$ LANGUAGE plpgsql;

CREATE TRIGGER trg_check_friendship
BEFORE INSERT OR UPDATE ON ShareStockList
FOR EACH ROW
EXECUTE FUNCTION check_friendship();

-- Insert initial data into the Stock table
INSERT INTO Stock(symbol)
SELECT DISTINCT symbol
FROM StockHistory
WHERE symbol NOT IN (SELECT symbol FROM Stock);