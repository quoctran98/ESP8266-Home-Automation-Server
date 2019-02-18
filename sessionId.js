var sessionId = document.getElementById('sessId').value;
sessionStorage.setItem('sessionId', sessionId);
document.getElementById('sessionIdSubmit').value = sessionId;
document.getElementById('sessionIdSubmit').click();