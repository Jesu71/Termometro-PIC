from pathlib import Path
import csv
import math
import sys

SD_IMAGE = Path(r"C:\Users\LENOVO\Documents\PICS\sdcard.mmc")

BLOCK_SIZE = 512
META_BLOCK = 99
DATA_START = 100


def read_block(file, block_number, size=BLOCK_SIZE):
    file.seek(block_number * BLOCK_SIZE)
    data = file.read(size)
    if len(data) != size:
        raise EOFError(f"No se pudo leer completo el bloque {block_number}.")
    return data


def read_metadata(path):
    with path.open("rb") as file:
        block = read_block(file, META_BLOCK)

    if block[0:3] != b"TM\x01":
        raise ValueError(
            "La firma de metadatos no coincide. "
            "Revisa que este sea el sdcard.mmc usado por Proteus."
        )

    return int.from_bytes(block[4:6], "little")


def parse_record(block):
    if block[0] != 0xA5 or block[1] != 0x01:
        return None

    return {
        "numero": int.from_bytes(block[2:4], "little"),
        "anio": block[4],
        "mes": block[5],
        "dia": block[6],
        "hora": block[7],
        "minuto": block[8],
        "segundo": block[9],
        "temperatura": block[10],
        "humedad": block[11],
    }


def read_records(path):
    total = read_metadata(path)
    rows = []

    with path.open("rb") as file:
        for i in range(total):
            block = read_block(file, DATA_START + i)
            record = parse_record(block)

            if record is not None:
                record["fecha_hora"] = (
                    f"20{record['anio']:02d}-"
                    f"{record['mes']:02d}-"
                    f"{record['dia']:02d} "
                    f"{record['hora']:02d}:"
                    f"{record['minuto']:02d}:"
                    f"{record['segundo']:02d}"
                )
                rows.append(record)

    rows.sort(key=lambda r: r["numero"])
    return total, rows


def percentile_pic(values, percent):
    values = sorted(int(v) for v in values)
    n = len(values)

    if n == 0:
        return None

    index = ((n - 1) * percent) // 100
    return values[index]


def statistics_pic(values):
    values = [int(v) for v in values]
    n = len(values)

    if n == 0:
        return {}

    mean = sum(values) // n
    variance = sum((v - mean) ** 2 for v in values) // n
    std = math.isqrt(variance)

    return {
        "Count": n,
        "Mean": mean,
        "Std": std,
        "Min": min(values),
        "25%": percentile_pic(values, 25),
        "50%": percentile_pic(values, 50),
        "75%": percentile_pic(values, 75),
        "Max": max(values),
    }


def forecast_pic(rows):
    if len(rows) < 2:
        return []

    previous = rows[-2]
    last = rows[-1]

    delta_t = last["temperatura"] - previous["temperatura"]
    delta_h = last["humedad"] - previous["humedad"]

    results = []

    for step in range(1, 7):
        pred_t = last["temperatura"] + delta_t * step
        pred_h = last["humedad"] + delta_h * step

        pred_t = max(0, min(50, pred_t))
        pred_h = max(0, min(100, pred_h))

        results.append({
            "paso": step,
            "temperatura": pred_t,
            "humedad": pred_h,
        })

    return results


def print_history(rows):
    print("\n" + "=" * 70)
    print("HISTORICO")
    print("=" * 70)

    if not rows:
        print("No hay registros validos.")
        return

    print(f"{'N':>4}  {'Fecha y hora':19}  {'Temp':>8}  {'Hum':>8}")
    print("-" * 70)

    for r in rows:
        print(
            f"{r['numero']:>4}  "
            f"{r['fecha_hora']:19}  "
            f"{r['temperatura']:>5} C  "
            f"{r['humedad']:>5} %"
        )


def print_statistics(rows):
    if not rows:
        return

    temp = statistics_pic(
        [r["temperatura"] for r in rows]
    )

    hum = statistics_pic(
        [r["humedad"] for r in rows]
    )

    print("\n" + "=" * 70)
    print("ESTADISTICAS")
    print("=" * 70)

    print(f"{'Medida':20}{'Temperatura':>15}{'Humedad':>15}")
    print("-" * 70)

    for key in [
        "Count", "Mean", "Std", "Min",
        "25%", "50%", "75%", "Max"
    ]:
        print(
            f"{key:20}"
            f"{temp[key]:>15}"
            f"{hum[key]:>15}"
        )


def print_forecast(rows):
    forecast = forecast_pic(rows)

    print("\n" + "=" * 70)
    print("PRONOSTICO — 6 HORAS")
    print("=" * 70)

    if not forecast:
        print("Se necesitan al menos 2 registros.")
        return

    print(f"{'Paso':>6}{'Temperatura':>20}{'Humedad':>15}")
    print("-" * 70)

    for item in forecast:
        print(
            f"{item['paso']:>6}"
            f"{item['temperatura']:>17} C"
            f"{item['humedad']:>13} %"
        )


def export_csv(rows):
    csv_path = SD_IMAGE.parent / "historico_termometro.csv"

    with csv_path.open(
        "w",
        newline="",
        encoding="utf-8"
    ) as file:
        writer = csv.writer(file)

        writer.writerow([
            "numero",
            "fecha_hora",
            "temperatura",
            "humedad"
        ])

        for r in rows:
            writer.writerow([
                r["numero"],
                r["fecha_hora"],
                r["temperatura"],
                r["humedad"]
            ])

    return csv_path


def main():
    print("=" * 70)
    print("ANALISIS DEL TERMOMETRO DIGITAL")
    print("=" * 70)

    print("\nSD:", SD_IMAGE)

    if not SD_IMAGE.exists():
        print("\nERROR: no se encontro el archivo sdcard.mmc")
        print("Ruta buscada:")
        print(SD_IMAGE)
        sys.exit(1)

    print(
        "Tamano:",
        SD_IMAGE.stat().st_size,
        "bytes"
    )

    try:
        total, rows = read_records(SD_IMAGE)
    except Exception as error:
        print("\nERROR LEYENDO SD:")
        print(error)
        sys.exit(1)

    print(
        "\nRegistros indicados por la SD:",
        total
    )

    print(
        "Registros validos encontrados:",
        len(rows)
    )

    print_history(rows)
    print_statistics(rows)
    print_forecast(rows)

    if rows:
        csv_path = export_csv(rows)

        print("\n" + "=" * 70)
        print("CSV GENERADO")
        print("=" * 70)
        print(csv_path)

    print("\nAnalisis terminado.")


if __name__ == "__main__":
    main()