import csv

count = 5
type = 2100

with open("./lichess_db_puzzle.csv", newline='') as db:
    #PuzzleId,FEN,Moves,Rating,RatingDeviation,Popularity,NbPlays,Themes,GameUrl
    reader = csv.reader(db,delimiter=",")
    
    with open("db.js",'w', newline='') as new_db:
        new_db.write("module.exports={\n")
        new_db.write("num_records: {},\n".format(count))
        new_db.write("type: '{}',\n".format(type)) 
        new_db.write("records:[\n")

        for row in reader: 
            row[7] = row[7].split(" ")
            row[2] = row[2].split(" ")
            if count > 0:
                if (type == "popular" and int(row[6]) > 1000) or type in row[7] or (isinstance(type, int) and int(row[3]) >= type):
                    new_db.write("{{fen:'{}',moves:{},rating:'{}',themes:{}}},\n".format(row[1], row[2], row[3], row[7]))
                    count -= 1

        new_db.write("]};")
