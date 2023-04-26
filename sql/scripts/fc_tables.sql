CREATE TABLE sensor_readings (
  id SERIAL PRIMARY KEY,
  reading_date DATE NOT NULL,
  reading_time TIME NOT NULL,
  water_temp DOUBLE PRECISION NOT NULL,
  depth DOUBLE PRECISION NOT NULL
);

CREATE TABLE fish_counter (
  id SERIAL PRIMARY KEY,
  fish_date DATE NOT NULL,
  fish_time TIME NOT NULL,
  fish_filename VARCHAR(255) NOT NULL,
  direction BOOLEAN,
  roi BOX
);

CREATE TABLE camera_settings (
  id SERIAL PRIMARY KEY,
  camera_date DATE NOT NULL,
  camera_time TIME NOT NULL,
  value JSON
);