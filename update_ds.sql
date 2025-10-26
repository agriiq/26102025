BEGIN TRANSACTION;
UPDATE data_source
SET secure_json_data = '{"token":"JNmI0z6JSdVU3-J1KwXGC7eM3gZovAgNpv0xEGgCxbU9ybHAzmR4Hzm-h9x5nnREDNH9OcXFqgIB0XYUCFVlWQ=="}'
WHERE uid = 'influxdb-flux';
COMMIT;
