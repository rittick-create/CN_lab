#!/usr/bin/env python3
"""Run repeatable end-to-end comparisons of the three ARQ protocols."""

import csv
import socket
import subprocess
import sys
import time
from pathlib import Path

ROOT = Path(__file__).resolve().parents[1]
PROTOCOLS = ["stopwait", "gobackn", "selective"]
PROBABILITIES = [0.0, 0.1, 0.2, 0.3, 0.4, 0.5]


def available_port():
    with socket.socket() as temporary_socket:
        temporary_socket.bind(("127.0.0.1", 0))
        return temporary_socket.getsockname()[1]


def parse_statistics(output):
    statistics = {}
    for line in output.splitlines():
        if line.startswith("STAT ") and "=" in line:
            key, value = line[5:].split("=", 1)
            statistics[key] = value
    return statistics


def run_case(protocol, probability, seed):
    port = available_port()
    output_file = ROOT / "Evaluation" / "received_output.txt"
    receiver = subprocess.Popen(
        [str(ROOT / "receiver_app"), str(port), str(output_file)],
        cwd=ROOT,
        stdout=subprocess.PIPE,
        stderr=subprocess.STDOUT,
        text=True,
    )
    time.sleep(0.1)

    sender_command = [
        str(ROOT / "sender_app"),
        protocol,
        str(ROOT / "sample_input.txt"),
        "4",
        str(probability),
        "127.0.0.1",
        str(port),
        str(seed),
    ]

    try:
        sender_result = subprocess.run(
            sender_command,
            cwd=ROOT,
            capture_output=True,
            text=True,
            timeout=90,
        )
        receiver_output, _ = receiver.communicate(timeout=10)
    except Exception:
        receiver.kill()
        receiver.wait()
        raise

    if sender_result.returncode != 0 or receiver.returncode != 0:
        raise RuntimeError(
            f"case failed: {protocol}, p={probability}\n"
            f"sender:\n{sender_result.stdout}{sender_result.stderr}\n"
            f"receiver:\n{receiver_output}"
        )

    if output_file.read_bytes() != (ROOT / "sample_input.txt").read_bytes():
        raise RuntimeError(f"received file mismatch: {protocol}, p={probability}")

    row = parse_statistics(sender_result.stdout)
    row["configured_probability"] = probability
    row["seed"] = seed
    return row


def main():
    repetitions = int(sys.argv[1]) if len(sys.argv) > 1 else 1
    rows = []

    for probability in PROBABILITIES:
        for protocol_number, protocol in enumerate(PROTOCOLS):
            for repetition in range(repetitions):
                seed = 1000 + protocol_number * 100 + repetition
                print(
                    f"Running {protocol:10s} p={probability:.1f} "
                    f"seed={seed}",
                    flush=True,
                )
                rows.append(run_case(protocol, probability, seed))

    results_file = ROOT / "Evaluation" / "results.csv"
    field_names = list(rows[0].keys())

    with results_file.open("w", newline="") as csv_file:
        writer = csv.DictWriter(csv_file, fieldnames=field_names)
        writer.writeheader()
        writer.writerows(rows)

    received_output = ROOT / "Evaluation" / "received_output.txt"
    if received_output.exists():
        received_output.unlink()

    print(f"All {len(rows)} cases passed. Results: {results_file}")


if __name__ == "__main__":
    main()
