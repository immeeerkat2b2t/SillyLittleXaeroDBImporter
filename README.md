### README.md

# Silly Little Xaero DB Importer

## Project Description

The Silly Little Xaero DB Importer is a command-line tool designed for silly gamers using the Xaero's Plus. It allows you to merge chunk highlight databases from different computers into one, making it easier to keep your maps synchronized. Map folder big, database small. Even when using sync software or rsync via a cron script I ran into the issue of mismatched chunk highlighting. Remove private coords from the DB and share with a friend!

## Installation Instructions

### Install WSL (Windows Subsystem for Linux)

1. Open Windows PowerShell as Administrator and run the following command:

    ```powershell
    wsl --install
    ```

2. Restart your computer when prompted. WSL will install Ubuntu by default.

3. Once your system restarts, open the Ubuntu terminal to complete the installation.

### Download Dependencies

1. Update your package list:

    ```bash
    sudo apt update
    ```

2. Install the required packages:

    ```bash
    sudo apt install build-essential libsqlite3-dev libncurses5-dev libncursesw5-dev
    ```

### Clone and Compile the Project

1. Clone the repository:

    ```bash
    git clone <repository-url>
    cd <repository-directory>
    ```

2. Compile the project:

    ```bash
    make
    ```

### Running the Application

1. Run the application:

    ```bash
    ./SillyLittleXaeroDBImporter
    ```

2. For help:

    ```bash
    ./SillyLittleXaeroDBImporter -h
    ```
TODO: help shit

3. After running the application you will be prompted with a list of commands. Type start and enter then enter the path of the database you want to import into IE  %userprofile%\AppData\blahblah\asatreat\Prisim\instance\2b2tminecraftinstance69\.minecraft\XaerosWorldMap\2b2tfolder\XaerosPlusNewChunks.db
4. You will be prompted for the path of the database you want to import from. Give da path and hit enter.
5. If you gave valid paths and the schema of the databases is the same, any row from the second database not currently in the first will be added. 
6. You may pause, resume, etc. 
7. ALWAYS BACKUP YOUR DATABASES PRIOR TO RUNNING!


## Poem

### The Silly Kitten on 2b2t

In the vast world of Minecraft, so grand and vast,
A silly kitten roamed, having a blast.
On the anarchy server, 2b2t,
She journeyed far, wild, and free.

She explored with joy, her map in hand,
Through forests, deserts, and icy land.
With each new step, her map grew wide,
A testament to her adventurous stride.

But this kitten, so kind and sweet,
Wanted her friends to see her feat.
She found a way, with code so neat,
To share her maps, a digital treat.

She merged her worlds with a clever tool,
Silly Little Xaero DB Importer, so cool.
Now her friends could see her journey bright,
In the endless world, day and night.

So if you wander the 2b2t,
Remember the kitten, wild and free.
With maps shared, adventures told,
Her legacy in the server, forever bold.

![Silly Kitten](https://placekitten.com/200/300)

## License

This project is licensed under the MIT License. See the LICENSE file for details.

---

