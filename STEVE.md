Here are other things left:

- ~~adding a tenant (u can add commands to be received via UART and then check wait for the input like name, and then add it to the list of tenants)~~
- tenant generating a code for their customers
- ~~a command inside the UART commands to ask tenants to pay rent. i've left some example code where you can start~~
- ~~washroom access based on temp codes or tenant passwords~~
- escalator sensors (try going through all the code and search for the code where i placed the buttons as entry or exit sensors). you can add them to an interrupt enabled pin and add an ISR (preferred and easier for you) or just put them next to the escalator LED and use my long method with the poll() function.