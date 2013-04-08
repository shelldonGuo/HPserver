
@echo "test init library...."
bin\test-init.exe

@echo "test timer...."
bin\test-timer.exe

@echo "test simple events...."
bin\test-sim-events.exe

@echo "test multimple events...."
bin\test-mul-events.exe


@echo "test multimple events in multiple threads...."
bin\test-threads.exe 3366

pause