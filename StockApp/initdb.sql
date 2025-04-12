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


WITH stock_returns AS (
    SELECT 
        symbol,
        timestamp,
        (close - LAG(close) OVER (PARTITION BY symbol ORDER BY timestamp)) / 
        LAG(close) OVER (PARTITION BY symbol ORDER BY timestamp) AS daily_return
    FROM StockHistory
    WHERE symbol IN (SELECT DISTINCT symbol FROM StockHistory)
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
INSERT INTO CachedStockStatistics (symbol, beta, correlation, last_updated)
SELECT 
    symbol,
    (covariance / NULLIF(market_variance, 0)) AS beta,
    (covariance / (SQRT(stock_variance) * SQRT(market_variance))) AS correlation,
    CURRENT_TIMESTAMP
FROM stats
ON CONFLICT (symbol)  -- If the symbol already exists, update the values
DO UPDATE SET 
    beta = EXCLUDED.beta,
    correlation = EXCLUDED.correlation,
    last_updated = CURRENT_TIMESTAMP;

CREATE TABLE CachedStockStatistics(
    symbol VARCHAR(10) NOT NULL,
    beta NUMERIC(10, 5),  -- Beta with precision for up to 5 decimal places
    correlation NUMERIC(10, 5),  -- Correlation coefficient with precision for up to 5 decimal places
    last_updated TIMESTAMP DEFAULT CURRENT_TIMESTAMP,  -- Track when the cache was last updated
    PRIMARY KEY (symbol)
);