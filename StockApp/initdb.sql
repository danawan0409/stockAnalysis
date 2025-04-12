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
    close NUMERIC(10, 2)
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
    quantity INTEGER NOT NULL,
    PRIMARY KEY (portfolioName, ownerUsername, stockID),
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
INSERT INTO Stock(symbol, close)
SELECT symbol, close
FROM StockHistory
WHERE timestamp = '2018-02-07';

-- Caching table
WITH stock_returns AS (
    SELECT 
        symbol,
        timestamp,
        (close - LAG(close) OVER (PARTITION BY symbol ORDER BY timestamp)) / 
        LAG(close) OVER (PARTITION BY symbol ORDER BY timestamp) AS daily_return
    FROM StockHistory
),
market_returns AS (
    SELECT 
        timestamp,
        AVG(daily_return) AS market_return
    FROM stock_returns
    GROUP BY timestamp
),
filtered AS (
    SELECT 
        sr.timestamp,
        sr.symbol,
        sr.daily_return AS stock_return,
        mr.market_return
    FROM stock_returns sr
    JOIN market_returns mr ON sr.timestamp = mr.timestamp
),
stats AS (
    SELECT 
        symbol,
        VAR_POP(stock_return) AS stock_variance,
        COVAR_POP(stock_return, market_return) AS covariance,
        VAR_POP(market_return) AS market_variance
    FROM filtered
    GROUP BY symbol
)
INSERT INTO CachedStockStatistics (symbol, beta, variation, last_updated)
SELECT 
    symbol,
    (covariance / NULLIF(market_variance, 0)) AS beta,
    stock_variance AS variation,
    CURRENT_TIMESTAMP
FROM stats
ON CONFLICT (symbol)
DO UPDATE SET 
    beta = EXCLUDED.beta,
    variation = EXCLUDED.variation,
    last_updated = CURRENT_TIMESTAMP;


CREATE TABLE CachedStockStatistics(
    symbol VARCHAR(10) NOT NULL,
    beta NUMERIC(10, 5),         -- Beta with precision for up to 5 decimal places
    variation NUMERIC(10, 5),    -- Stock variance instead of correlation
    last_updated TIMESTAMP DEFAULT CURRENT_TIMESTAMP,
    PRIMARY KEY (symbol)
);

CREATE TABLE CachedMatrix (
    symbol1 VARCHAR(10),
    symbol2 VARCHAR(10),
    correlation NUMERIC(15, 6),
    covariance NUMERIC(15, 6),
    last_updated TIMESTAMP DEFAULT CURRENT_TIMESTAMP,
    PRIMARY KEY (symbol1, symbol2)
);


INSERT INTO CachedMatrix (symbol1, symbol2, correlation, covariance, last_updated)
WITH stock_prices AS (
    SELECT symbol, timestamp, close
    FROM StockHistory
),
returns AS (
    SELECT 
        symbol,
        timestamp,
        (close - LAG(close) OVER (PARTITION BY symbol ORDER BY timestamp)) / 
        NULLIF(LAG(close) OVER (PARTITION BY symbol ORDER BY timestamp), 0) AS daily_return
    FROM stock_prices
),
valid_returns AS (
    SELECT * FROM returns WHERE daily_return IS NOT NULL
),
paired_returns AS (
    SELECT 
        a.symbol AS symbol1,
        b.symbol AS symbol2,
        a.daily_return AS ret1,
        b.daily_return AS ret2
    FROM valid_returns a
    JOIN valid_returns b ON a.timestamp = b.timestamp
)
SELECT 
    symbol1,
    symbol2,
    CORR(ret1, ret2) AS correlation,
    COVAR_POP(ret1, ret2) AS covariance,
    CURRENT_TIMESTAMP
FROM paired_returns
GROUP BY symbol1, symbol2
ON CONFLICT (symbol1, symbol2) DO UPDATE
SET correlation = EXCLUDED.correlation,
    covariance = EXCLUDED.covariance,
    last_updated = CURRENT_TIMESTAMP;
