from locust import HttpUser, task, between

class WebServerUser(HttpUser):
    @task
    def index_page(self):
        self.client.get("/")

    @task
    def page_2(self):
        self.client.get("/page2.html")
