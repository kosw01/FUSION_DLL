import pandas as pd
import matplotlib.pyplot as plt
from matplotlib.ticker import MaxNLocator

input_data = pd.read_csv('bin/input.csv')
output_data = pd.read_csv('bin/output.csv')

def graph(dir):
  fig, ax = plt.subplots(3, 1, figsize=(10, 5))
  ax[0].plot(input_data['DateTime'][::10], input_data[f'GPS_{dir}'][::10], label='Input')  # 100Hz x축, 10Hz y축 샘플링
  ax[1].plot(output_data['DateTime'], output_data[f'Displacement_{dir}'], label='Output')
  ax[2].plot(output_data['DateTime'], output_data[f'Displacement_{dir}'], label='Output')
  ax[2].plot(input_data['DateTime'][::10], input_data[f'GPS_{dir}'][::10], label='Input')
  
  for a in ax:
      a.xaxis.set_major_locator(MaxNLocator(nbins=4))
      a.tick_params(axis='x', rotation=0)
    
  plt.legend()
  plt.show()

graph('Z')
graph('Y')
