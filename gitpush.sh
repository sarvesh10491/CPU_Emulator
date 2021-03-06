git add -A

read -p "Enter the commit message : " commitMessage 
git commit -m "$commitMessage"

read -p "Enter the name of the branch : " branch
git push origin $branch