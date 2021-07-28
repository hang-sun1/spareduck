import csv

count = 0
with open("./lichess_db_puzzle.csv", newline='') as db:
    reader = csv.reader(db,delimiter=",")
    with open("db.csv",'w', newline='') as new_db:
        writer = csv.writer(new_db, delimiter=",")
        for row in reader:  #PuzzleId,FEN,Moves,Rating,RatingDeviation,Popularity,NbPlays,Themes,GameUrl
            if count < 100 and int(row[6]) > 1000:
                writer.writerow([row[1], row[2], row[3], row[7]])
                count += 1
