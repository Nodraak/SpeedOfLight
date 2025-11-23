import random


# high samples per rev (to test debounce)
HIGH_COUNT = 1
# HIGH_COUNT = 10

RPM = 1000
SAMPLES_PER_REV = 60


def gen_samples(avg, duration_sec):
    samples = []  # 0-4096

    for i in range(int(SAMPLES_PER_REV * RPM / 60 * duration_sec)):
        pos = i % SAMPLES_PER_REV
        if pos < HIGH_COUNT:
            mu = avg + 500
        else:
            mu = avg - 500
        s = int(random.gauss(mu=mu, sigma=100))
        assert (0 <= s) and (s < 4096)
        samples.append(s)

    return samples


def main():
    print(f"HIGH_COUNT={HIGH_COUNT}")
    print(f"RPM={RPM} ({RPM/60:.0f} Hz)")
    print(f"SAMPLES_PER_REV={SAMPLES_PER_REV}")
    print(f"samples per sec = {RPM/60*SAMPLES_PER_REV:.0f}")
    print("Generating...")

    #
    # Gen
    #

    samples = []  # 0-4096

    samples += gen_samples(avg=1000, duration_sec=5) # 0-1000 / 1000-2000
    samples += gen_samples(avg=3000, duration_sec=5) # 2000-3000 / 3000-4000

    #
    # Print
    #

    s = sum(samples)
    n = len(samples)

    print(f"=> sum={s} / sample_total_count={n} = average={int(s/n)}")
    print("Writing to adc-data.txt")

    with open("adc-data.txt", "w") as f:
        for s in samples:
            f.write("%d\n" % s)


main()
