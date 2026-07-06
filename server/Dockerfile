FROM node:20.1-alpine3.16
WORKDIR /app
EXPOSE 3030
COPY . .

RUN npm install
CMD ["node", "server.js"]
