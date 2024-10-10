import numpy as np
from scipy.signal import freqz
import matplotlib.pyplot as plt

fs = 16000
freqs = np.linspace(20, fs/2, 1000)

for x in np.linspace(.1, .75, 10):
    eq = np.array([x, 1, x])
    eq = eq / np.sum(eq)
    w, h = freqz(eq, [1], fs=fs, worN=freqs)
    hdb = 20*np.log10(np.abs(h))
    plt.plot(w, hdb, label=",".join([f"{x:.2}" for x in eq]), linewidth=3)
plt.grid()
plt.ylim(-60, 10)
plt.legend()
plt.show()
