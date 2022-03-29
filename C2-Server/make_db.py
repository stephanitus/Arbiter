from app import db
db.create_all()

from app import Operator
root = Operator(username='root', password='root')
db.session.add(root)
db.session.commit()