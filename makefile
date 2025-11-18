all: controlador cliente veiculo

controlador: controlador.c comum.h
	gcc -o controlador controlador.c

cliente: cliente.c comum.h
	gcc -o cliente cliente.c

veiculo: veiculo.c comum.h
	gcc -o veiculo veiculo.c

clean:
	rm -f controlador cliente veiculo *.o