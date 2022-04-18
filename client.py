import socket      
import pandas as pd
import plotly.express as px

def sign_extend(value, bits):
    sign_bit = 1 << (bits - 1)
    return (value & (sign_bit - 1)) - (value & sign_bit)

def recv_data(transfer_size, ip, port, raw, gain, coupling, decimation_factor, packet_length):
	g = [1 if value == "HIGH" else 0 for value in gain]
	c = [1 if value == "DC" else 0 for value in coupling]

	s = socket.socket(socket.AF_INET, socket.SOCK_STREAM)            
	s.setsockopt(socket.SOL_SOCKET, socket.SO_RCVBUF, 65536)
	print("Searching for connection...")
	s.connect((ip, port))
	print("Connection found!")
	configuration = transfer_size.to_bytes(4, 'big') + ((g[1]<<1)+g[0]).to_bytes(1,'big') + ((c[1]<<1)+c[0]).to_bytes(1, 'big') + decimation_factor.to_bytes(4, 'big') + packet_length.to_bytes(4, 'big')
	print("Sending configuration...")
	s.send(configuration)
	print("Finished sending configuration!")

	with open(raw, 'wb') as f:
		print("Receiving data...")
		while(True):
			data = s.recv(65536)
			if(data==b''):
				break
			f.write(data)

	print("Finished receiving data!")
	s.close()  
	f.close()

def format_data(raw, csv, resolution, gain):
	print("Formatting data...")

	rangeCh1 = 1 if gain[1] == "HIGH" else 25
	rangeCh2 = 1 if gain[0] == "HIGH" else 25

	with open(raw, 'rb') as fb:
		with open(csv, 'w') as f:
			data = fb.read(4)
			while data:
				f.write(f"""{sign_extend(int.from_bytes(data[0:2], byteorder='little', signed=False) >> (16-resolution) & 0x3FFF, resolution) * rangeCh1 / 2**13}, {
					sign_extend(int.from_bytes(data[2:4], byteorder='little', signed=False) >> (16-resolution) & 0x3FFF, resolution) * rangeCh2 / 2**13}\n""")
				data = fb.read(4)
		f.close()
	fb.close()
	print("Finished formatting data!")

def display_data(raw, resolution, gain):
	print("Displaying data...")

	rangeCh1 = 1 if gain[1] == "HIGH" else 25
	rangeCh2 = 1 if gain[0] == "HIGH" else 25

	data_to_plot = []
	with open(raw, 'rb') as fb:
		data = fb.read(4)
		while data:
			data_to_plot.append([sign_extend(int.from_bytes(data[0:2], byteorder='little', signed=False) >> (16-resolution) & 0x3FFF, resolution) * rangeCh1 / 2**13, sign_extend(int.from_bytes(data[2:4], byteorder='little', signed=False) >> (16-resolution) & 0x3FFF, resolution) * rangeCh2 / 2**13])			
			data = fb.read(4)
	fb.close()

	df = pd.DataFrame(data_to_plot, columns=['Channel 2', 'Channel 1'])
	fig = px.line(df, x=df.index, y=df.columns, title='Zmod Samples')
	fig.show()
	print("Finished displaying data!")


def main(args):
	recv_data(int(args.transfer), args.ip, int(args.port), args.raw, args.gain, args.coupling, int(args.decimation), int(args.packet))
	if(args.plot):
		display_data(args.raw, int(args.resolution), args.gain)
	if(args.format):
		format_data(args.raw, args.csv, int(args.resolution), args.gain)
		
if __name__ == '__main__':
	import argparse

	parser = argparse.ArgumentParser(description="Log samples acquired by a Zmod Scope")
	parser.add_argument('-t', '--transfer', metavar='<transfer size>', required=True,
	                    help='Required. Number of samples to be transferred from the Zmod. This value will be rounded up to the closest multiple of the packet length.')
	parser.add_argument('-ip', metavar='<address>', required=True,
	                    help='Required. IP address of the board containing the Zmod')
	parser.add_argument('-r', '--resolution', metavar='<resolution>', type=int, required=True, choices=(10, 12, 14),
						help='Required. Resolution of the Zmod.')
	parser.add_argument('-g', '--gain', metavar=('<gain channel 1>','<gain channel 2>'), nargs=2, default=("LOW", "LOW"), type=str, choices=("LOW", "HIGH"),
						help='Optional. HIGH/LOW Gain of the Zmod Channels. Default is LOW for both channels.')
	parser.add_argument('-c', '--coupling', metavar=('<coupling channel 1>','<coupling channel 2>'), nargs=2, default=("AC", "AC"), type=str, choices=("AC", "DC"),
						help='Optional. AC/DC coupling for the Zmod. Default is AC for both channels.')
	parser.add_argument('-raw', metavar='<path>',
	           			help='Optional. Specify file to store raw data. Default is "output.raw".',
	           			default='output.raw')
	parser.add_argument('-port', metavar='<port>', default=8082, type=int,
	                    help='Optional. Port for the connection. Default is 8082.')
	parser.add_argument('-d', '--decimation', metavar='<factor>', default=10, type=int, choices=range(0, 32768),
	                    help='Optional. Decimation factor. Default is 10.')
	parser.add_argument('-p', '--packet', metavar='<length>', default=16384, type=int, choices=range(0, 65536),
	                    help='Optional. Packet length. Default is 16384.')
	parser.add_argument('-csv', metavar='<path>',
						help='Optional. Specify csv file to store formatted data, if -f/--format flag is provided. Default is "output.csv".',
						default='output.csv')
	parser.add_argument('-f', '--format', help='Optional. Format data from into a csv file specified by -csv.', action='store_true')
	parser.add_argument('-P', '--plot', help='Optional. Plot transferred data.', action='store_true')

	args = parser.parse_args()
	main(args)