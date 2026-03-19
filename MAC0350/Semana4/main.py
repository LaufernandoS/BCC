from fastapi import FastAPI, HTTPException
from fastapi.staticfiles import StaticFiles
from pydantic import BaseModel
from typing import Any


app = FastAPI()

app.mount("/", StaticFiles(directory="src",html = True), name="src")

users = ["jaja", "jeje", "jojo"]

# class User(BaseModel):
#     idx: int
#     name: str

@app.get("/")
async def read_root():
    return {"Hello": "World"}

@app.get("/users")
async def list_users():
    return users

@app.get("/users/{index}")
async def users(index: int):
    return {"nome": users[index]}

# @app.post("/users", status_code=201)
# async def create_user(user: User):
#     users.append(user)
#     return user

# @app.get("/users/{user_id}")
# async def get_user(user_id: int):
#     for user in users:
#         if user.id == user_id:
#             return user
#     raise HTTPException(status_code=404, detail="User not found")

# @app.put("/users/{user_id}")
# async def update_user(user_id: int, updated_user: User):
#     for index, user in enumerate(users):
#         if user.id == user_id:
#             users[index] = updated_user
#             return updated_user
#     raise HTTPException(status_code=404, detail="User not found")




# @app.get("/users/{index}")
# def get_user(idx: int):
#     """
#     Retrieve a user by their idx (0-based).
#     """
#     if 0 <= idx < len(users):
#         return users[idx]
#     return {"error": "User not found"}

# @app.post("/users/")
# async def update_user(username: str, user: User):
#     return {"message": "User created successfully", "username": user.name, "user_idx": user.idx}


# @app.get("/items/{item_id}")
# def read_item(item_id: int, q: str | None = None):
#     return {"item_id": item_id, "q": q}

