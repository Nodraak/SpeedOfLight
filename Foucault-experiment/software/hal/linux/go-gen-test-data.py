import random

# high samples per period
HIGH_COUNT = 1
# HIGH_COUNT = 10

N_SAMPLES = 1000
PERIOD_MS = 50 # 50 ms = 20 Hz = 1200 RPM


def gen_samples(offset):
    samples = []  # 0-4096

    for i in range(N_SAMPLES):
        pos = i % PERIOD_MS
        if pos < HIGH_COUNT:
            samples.append(random.randint(offset + 1100, offset + 2000))
        else:
            samples.append(random.randint(offset + 100, offset + 900))

    return samples


def main():
    #
    # Gen
    #

    samples = []  # 0-4096

    for _ in range(5):
        samples += gen_samples(offset=0)
    for _ in range(5):
        samples += gen_samples(offset=2000)

    #
    # Print
    #

    s = sum(samples)
    n = len(samples)
    print(f"=> sum={s} / sample_total_count={n} =  average={int(s/n)}")

    for s in samples:
        print(s)


main()
