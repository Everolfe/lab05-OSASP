
# README: Производители и потребители с использованием очереди и семафоров

## Описание
Этот проект реализует многопроцессную систему производителей и потребителей, использующую общую память и семафоры для синхронизации. Производители создают сообщения и добавляют их в очередь, а потребители извлекают сообщения и проверяют их целостность.

## Функциональность
- Использование очереди в разделяемой памяти для хранения сообщений.
- Синхронизация процессов с помощью семафоров.
- Динамическое создание и остановка производителей и потребителей.
- Обработка сигналов для корректного завершения работы.
- Отображение состояния системы.

## Управление
- `p` - создать производителя
- `c` - создать потребителя
- `r` - остановить производителя
- `d - остановить потребителя
- `s` - показать статус очереди
- `q` - завершить работу

## Завершение работы
Программа может быть завершена вручную с помощью `q` или сигнала `SIGINT` (`Ctrl+C`). Все процессы и ресурсы освобождаются при выходе.

## Зависимости
- `unistd.h`, `sys/types.h`, `sys/ipc.h`, `sys/shm.h`, `sys/sem.h`, `sys/wait.h`, `signal.h`, `stdio.h`, `stdlib.h`, `stdbool.h`, `string.h`, `fcntl.h`, `time.h`, `termios.h`

## Возможные улучшения
- Логирование событий в файл
- Поддержка многопоточности вместо процессов
- Настройка параметров через конфигурационный файл